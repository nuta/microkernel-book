#pragma once
#include <libs/common/types.h>

// リストを初期化する。list_init関数の代替。static変数でリストを宣言した場合に便利。
#define LIST_INIT(list)                                                        \
    { .prev = &(list), .next = &(list) }

// リストから先頭のエントリを取り出す。containerはエントリを含む構造体、fieldはlist_elem_tの
// フィールド名。list_pop_front関数とは違い、list_elem_tへのポインタではなくそれを内包する
// 構造体 (container) へのポインタを返す。エントリがない場合はNULLを返す。
#define LIST_POP_FRONT(list, container, field)                                 \
    ({                                                                         \
        list_elem_t *__elem = list_pop_front(list);                            \
        (__elem) ? LIST_CONTAINER(__elem, container, field) : NULL;            \
    })

// list_elem_tへのポインタ (elem) から、それを含む構造体 (container) へのポインタを取得する。
#define LIST_CONTAINER(elem, container, field)                                 \
    ((container *) ((vaddr_t) (elem) -offsetof(container, field)))

// リストの各エントリに対する、いわゆる foreach 文を実現するマクロ。elem はforeach文で
// 使う任意の変数名、list はリスト、container はエントリを含む構造体、field は
// list_elem_tのフィールド名。
//
// 使い方:
//
//    struct element {
//        list_elem_t next;
//        int foo;
//    };
//
//    LIST_FOR_EACH(elem, &elems, struct element, next) {
//        printf("foo: %d", elem->foo);
//    }
//
// なお __next を取り出しているのは、foreach文中で elem が削除された場合に elem->next が
// 無効になるため。
#define LIST_FOR_EACH(elem, list, container, field)                            \
    for (container *elem = LIST_CONTAINER((list)->next, container, field),     \
                   *__next = NULL;                                             \
         (&elem->field != (list)                                               \
          && (__next = LIST_CONTAINER(elem->field.next, container, field)));   \
         elem = __next)

// リスト (intrusiveデータ構造)
struct list {
    struct list *prev;  // 末尾 (list_t) または 前のエントリ (list_elem_t)
    struct list *next;  // 先頭 (list_t) または 次のエントリ (list_elem_t)
};

typedef struct list list_t;       // リスト
typedef struct list list_elem_t;  // リストのエントリ

void list_init(list_t *list);
void list_elem_init(list_elem_t *elem);
bool list_is_empty(list_t *list);
bool list_is_linked(list_elem_t *elem);
size_t list_len(list_t *list);
bool list_contains(list_t *list, list_elem_t *elem);
void list_remove(list_elem_t *elem);
void list_push_back(list_t *list, list_elem_t *new_tail);
list_elem_t *list_pop_front(list_t *list);
