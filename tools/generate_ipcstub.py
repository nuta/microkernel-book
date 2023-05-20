"""

IPCスタブジェネレータ

"""
import sys
import argparse
import jinja2
from glob import glob
from lark import Lark
from lark.exceptions import LarkError
from colorama import Fore, Style

parser = Lark(
    r"""
%import common.WS
%ignore WS
%ignore COMMENT
%ignore EMPTY_COMMENT
COMMENT: /\/\/[^\/][^\n]*/
EMPTY_COMMENT: /\/\/[^\/]/
NAME: /[a-zA-Z_][a-zA-Z_0-9]*/
PATH: /"[^"]+"/
NATINT: /[1-9][0-9]*/

start: stmts?
stmts: stmt*
stmt:
    | doc_comment
    | msg_def

doc_comment: "///" /[^\n]*\n/
msg_def: modifiers? MSG_TYPE NAME "(" fields ")" ["->" "(" fields ")"] ";"
modifiers: MODIFIER*
MSG_TYPE: "rpc" | "oneway"
MODIFIER: "async"
any_fields: "any"
fields: any_fields | (field ("," field)*)?
field: NAME ":" type
type: NAME ("[" NATINT "]")?
"""
)


class ParseError(Exception):
    def __init__(self, message, hint=None):
        self.message = message
        self.hint = hint


next_msg_id = 1


class IDLParser:
    def __init__(self):
        self.messages = []
        self.message_context = None
        self.prev_stmt = ""

    def parse(self, text):
        ast = parser.parse(text)
        for stmt in ast.children[0].children:
            self.visit_stmt(stmt)

        return {
            "messages": self.messages,
        }

    def visit_stmt(self, tree):
        assert tree.data == "stmt"
        stmt = tree.children[0]
        if stmt.data == "msg_def":
            msg_def = self.visit_msg_def(stmt)
            self.messages.append(msg_def)
        elif stmt.data == "doc_comment":
            pass
        else:
            # unreachable
            assert False
        self.prev_stmt = stmt

    def visit_msg_def(self, tree):
        global next_msg_id
        assert tree.data == "msg_def"
        name = tree.children[2].value
        self.message_context = name

        modifiers = list(map(lambda x: x.value, tree.children[0].children))
        msg_type = tree.children[1].value
        name = tree.children[2].value
        args = self.visit_fields(tree.children[3])
        id = next_msg_id
        next_msg_id += 1

        is_oneway = "oneway" == msg_type or "async" in modifiers
        if len(tree.children) > 4 and tree.children[4] is not None:
            rets = self.visit_fields(tree.children[4])
            reply_id = next_msg_id
            next_msg_id += 1
        else:
            if not is_oneway:
                raise ParseError(
                    f"{name}: return values is not specified",
                    "Add '-> ()' or consider defining it as 'oneway' message",
                )
            rets = []
            reply_id = None

        msg_def = {
            "id": id,
            "reply_id": reply_id,
            "name": name,
            "oneway": is_oneway,
            "modifiers": modifiers,
            "msg_type": msg_type,
            "args": args,
            "rets": rets,
        }

        self.message_context = None
        return msg_def

    def visit_fields(self, tree):
        if len(tree.children) == 1 and tree.children[0].data == "any_fields":
            return {
                "fields": []
            }

        fields = []
        for tree in tree.children:
            field = self.visit_field(tree)
            fields.append(field)

        return {
            "fields": fields,
        }

    def visit_field(self, tree):
        assert tree.data == "field"
        return {
            "name": tree.children[0].value,
            "type": self.visit_type(tree.children[1]),
        }

    def visit_type(self, tree):
        assert tree.data == "type"
        if len(tree.children) > 1:
            nr = int(tree.children[1].value)
        else:
            nr = None

        return {
            "name": tree.children[0].value,
            "nr": nr,
        }


def generate(args, idl):
    builtins = dict(
        char="char",
        bool="bool",
        int="int",
        uint="unsigned",
        int8="int8_t",
        int16="int16_t",
        int32="int32_t",
        int64="int64_t",
        uint8="uint8_t",
        uint16="uint16_t",
        uint32="uint32_t",
        uint64="uint64_t",
        size="size_t",
        offset="offset_t",
        uaddr="uaddr_t",
        paddr="paddr_t",
        task="task_t",
        notifications="notifications_t",
    )

    def resolve_type(type_):
        resolved_type = builtins.get(type_["name"])
        if resolved_type is None:
            raise ParseError(f"Unknown data type: '{type_['name']}'")
        return resolved_type

    def field_defs(fields):
        defs = []
        for field in fields:
            type_ = field["type"]
            if type_["name"] == "bytes":
                defs.append(f"uint8_t {field['name']}[{type_['nr']}]")
                defs.append(f"size_t {field['name']}_len")
            elif type_["name"] == "cstr":
                defs.append(f"char {field['name']}[{type_['nr']}]")
            else:
                def_ = resolve_type(type_)
                def_ += f" {field['name']}"
                if type_["nr"]:
                    def_ += f"[{type_['nr']}]"
                defs.append(def_)
        return defs

    renderer = jinja2.Environment()
    renderer.filters["newlines_to_whitespaces"] = lambda text: text.replace("\n", " ")
    renderer.filters["field_defs"] = field_defs
    template = renderer.from_string(
        """\
#pragma once
//
//  generate_ipcstub.py で自動生成されたIPCスタブ。
//
//  このファイルを直接編集しないこと。代わりに messages.idl や tools/generate_ipcstub.py を
//  変更するべき。
//
#include <libs/common/types.h>

//
//  メッセージフィールドの定義
//
{% for msg in messages %}
struct {{ msg.name }}_fields {{ "{" }}
{%- for field in msg.args.fields | field_defs %}
    {{ field }};
{%- endfor %}
{{ "};" }}

{%- if not msg.oneway %}
struct {{ msg.name }}_reply_fields {{ "{" }}
{%- for field in msg.rets.fields | field_defs %}
    {{ field }};
{%- endfor %}
{{ "};" }}
{%- endif %}
{% endfor %}

#define __DEFINE_MSG_TYPE(type, len) ((type) << 12 | (len))

{% for msg in messages %}
#define {{ msg.name | upper }}_MSG __DEFINE_MSG_TYPE({{ msg.id }}, sizeof(struct {{ msg.name }}_fields))
{%- if not msg.oneway %}
#define {{ msg.name | upper }}_REPLY_MSG __DEFINE_MSG_TYPE({{ msg.reply_id }}, sizeof(struct {{ msg.name }}_fields))
{%- endif %}
{%- endfor %}

//
//  各種マクロの定義
//
#define IPCSTUB_MESSAGE_FIELDS \\
{%- for msg in messages %}
    struct {{ msg.name }}_fields {{ msg.name }}; \\
{%- if not msg.oneway %}
    struct {{ msg.name }}_reply_fields {{ msg.name }}_reply; \\
{%- endif %}
{%- endfor %}

#define IPCSTUB_MSGID_MAX {{ msgid_max }}
#define IPCSTUB_MSGID2STR \\
    (const char *[]){{ "{" }} \\
    {% for m in messages %} \\
        [{{ m.id }}] = "{{ m.name }}", \\
        {%- if not m.oneway %}
        [{{ m.reply_id }}] = "{{ m.name }}_reply", \\
        {%- endif %}
    {% endfor %} \\
    {{ "}" }}

#define IPCSTUB_STATIC_ASSERTIONS \\
{%- for msg in messages %}
    _Static_assert( \\
        sizeof(struct {{ msg.name }}_fields) < 4096, \\
        "'{{ msg.name }}' message is too large, should be less than 4096 bytes" \\
    ); \\
{%- if not msg.oneway %}
    _Static_assert( \\
        sizeof(struct {{ msg.name }}_reply_fields) < 4096, \\
        "'{{ msg.name }}_reply' message is too large, should be less than 4096 bytes" \\
    ); \\
{%- endif %}
{%- endfor %}


"""
    )

    msgid_max = next_msg_id - 1
    text = template.render(msgid_max=msgid_max, **idl)

    with open(args.out, "w", encoding="utf-8") as f:
        f.write(text)


def main():
    parser = argparse.ArgumentParser(description="The IPC stubs generator.")
    parser.add_argument("idl", help="The IDL file.")
    parser.add_argument(
        "-o",
        dest="out",
        required=True,
        help="The output file.",
    )
    args = parser.parse_args()

    idl_text = open(args.idl, encoding="utf-8").read()

    try:
        idl = IDLParser().parse(idl_text)
        generate(args, idl)
    except ParseError as e:
        print(
            f"ipc_stub: {Fore.RED}{Style.BRIGHT}error:{Fore.RESET} {e.message}{Style.RESET_ALL}"
        )
        if e.hint is not None:
            print(
                f"{Fore.YELLOW}{Style.BRIGHT}    | Hint:{Fore.RESET} {e.hint}{Style.RESET_ALL}"
            )
        sys.exit(1)
    except LarkError as e:
        print(
            f"ipc_stub: {Style.BRIGHT}{Fore.RED}error:{Fore.RESET} {e}{Style.RESET_ALL}"
        )


if __name__ == "__main__":
    main()
