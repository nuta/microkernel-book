#include <libs/common/list.h>
#include <libs/common/print.h>

// prev と next の間に新しいエントリ new を挿入する。
//
//    prev <-> next  =>  prev <-> new <-> next
//
static void list_insert(list_elem_t *prev, list_elem_t *next,
                        list_elem_t *new) {
    new->prev = prev;
    new->next = next;
    next->prev = new;
    prev->next = new;
}

// エントリを無効な状態にする。バグ検出用。
static void list_elem_nullify(list_elem_t *elem) {
    elem->prev = NULL;
    elem->next = NULL;
}

// リストを初期化する。初期状態ではリストは空。
void list_init(list_t *list) {
    list->prev = list;
    list->next = list;
}

// リストのエントリを初期化する。
void list_elem_init(list_elem_t *elem) {
    list_elem_nullify(elem);
}

// リストが空かどうかを返す。
bool list_is_empty(list_t *list) {
    return list->next == list;
}

// リストのエントリがどこかのリストに所属しているか (つまり使用中か) を返す。O(1)。
bool list_is_linked(list_elem_t *elem) {
    return elem->next != NULL;
}

// リストの総エントリ数を返す。O(n)。
size_t list_len(list_t *list) {
    size_t len = 0;
    struct list *node = list->next;
    while (node != list) {
        len++;
        node = node->next;
    }

    return len;
}

// リストに指定されたエントリが含まれているかを返す。O(n)。
bool list_contains(list_t *list, list_elem_t *elem) {
    list_elem_t *node = list->next;
    while (node != list) {
        if (node == elem) {
            return true;
        }

        node = node->next;
    }

    return false;
}

// リストからエントリを削除する。O(1)。
void list_remove(list_elem_t *elem) {
    if (!list_is_linked(elem)) {
        return;
    }

    // 前後のエントリをつなぎ直すことで、エントリをリストから削除する。
    //
    //   prev <-> elem <-> next => prev <-> next
    //
    elem->prev->next = elem->next;
    elem->next->prev = elem->prev;

    // エントリを無効な状態にする。こうすることで二回以上の削除を防ぐ。
    list_elem_nullify(elem);
}

// エントリをリストの末尾に追加する。O(1)。
void list_push_back(list_t *list, list_elem_t *new_tail) {
    DEBUG_ASSERT(!list_contains(list, new_tail));
    DEBUG_ASSERT(!list_is_linked(new_tail));
    list_insert(list->prev, list, new_tail);
}

// リストの先頭エントリを取り出す。空の場合はNULLを返す。O(1)。
list_elem_t *list_pop_front(list_t *list) {
    struct list *head = list->next;
    if (head == list) {
        return NULL;
    }

    // エントリをリストから削除する。
    struct list *next = head->next;
    list->next = next;
    next->prev = list;

    // エントリを無効な状態にする。こうすることで list_remove() を呼び出しても削除しない
    // ようにする。
    list_elem_nullify(head);
    return head;
}
