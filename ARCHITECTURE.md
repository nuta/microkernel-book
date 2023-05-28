# HinaOSの内部構造

本ドキュメントは、ソースコードのファイル構成や動作の流れを通じて、HinaOSの土地勘をつかむことを目的としています。

## リポジトリの構成

```
.
├── fs                -- HinaFSの内容
├── kernel            -- カーネル
│   └── riscv32       -- カーネルのCPUアーキテクチャ依存部分 (32ビットRISC-V用)
├── libs              -- ライブラリ
│   ├── common        -- カーネル・ユーザーランド共通のライブラリ
│   │   └── riscv32   -- commonライブラリのCPUアーキテクチャ依存部分 (32ビットRISC-V用)
│   └── user          -- ユーザーランド用ライブラリ
│       ├── riscv32   -- userライブラリのCPUアーキテクチャ依存部分 (32ビットRISC-V用)
│       └── virtio    -- virtioデバイスドライバのためのライブラリ
├── mk                -- ビルドシステム関連
├── servers           -- サーバ・アプリケーション
│   ├── crack         -- HinaOSの脆弱性を実証するプログラム
│   ├── fs            -- HinaFSファイルシステムサーバ
│   ├── hello         -- Hello Worldを表示するプログラム
│   ├── hello_hinavm  -- HinaVM上でサンプルプログラム (pongサーバ) を起動するプログラム
│   ├── pong          -- pongサーバ (シェルのpingコマンドの通信先)
│   ├── shell         -- コマンドラインシェル
│   ├── tcpip         -- TCP/IPサーバ
│   ├── virtio_blk    -- virtio-blk ストレージデバイスドライバ
│   ├── virtio_net    -- virtio-net ネットワークデバイスドライバ
│   └── vm            -- VMサーバ: メモリ管理、タスク管理など (最初に起動するサーバ)
└── tools             -- 雑多なビルド・開発用スクリプト
```

## HinaOSカーネルの基本動作

カーネルはイベント駆動型で動作します。「イベント」にはシステムコール、ページフォルトを始めとする例外、キーボードなど周辺デバイスからの割り込みなどがあります。

イベントが発生すると、CPUは予めカーネルが設定しておいたイベントハンドラ (`riscv32_trap_handler`) に処理を移行します。カーネルは実行状態を保存した後にイベントに応じた処理を行い、ユーザータスクに処理を戻します。実行中タスクがブロック状態に入ったり、割り当てられたCPU時間を使い切ったりした場合は、コンテキストスイッチを行い、別のタスクを実行します。

また、カーネルの処理を始める際に「ビッグカーネルロック ([Wikipedia](https://ja.wikipedia.org/wiki/ジャイアントロック))」と呼ばれる単一ロックを取ります。そのため、カーネルのコードは基本的に1つのCPUでしか実行されません。この単純な仕組みのおかげで、ロックを意識せずにカーネルを書けるようになっています。

## アプリケーション・サーバの基本動作

アプリケーションやサーバも、カーネルと同じようにシングルスレッド・イベント駆動型で動作します。特にサーバでは、クライアントタスクからのメッセージの受信、メッセージの種類に応じた処理の実行、クライアントタスクへのメッセージの返信という流れが基本となります。

各サーバ・アプリケーションはVMサーバから起動されます。VMサーバは各ユーザータスクの「ページャタスク」として、ページフォルトや異常処理 (無効なポインタの参照など) といったイベントを処理するのが仕事です。

タスク同士はメッセージパッシング経由で通信しあいます。メッセージは単純なバイト列であり、メッセージの種類や内容は `messages.idl` というHinaOS独自のシンプルなインターフェイス記述言語で定義されます。また、メッセージパッシングとは別に通知IPC (notifications) という仕組みもカーネルは提供しています。

メッセージパッシングは同期的に行われ、メッセージの送信処理は宛先タスクが受信状態に入るまでブロックします。ただし、タスク同士が互いにメッセージを同期的に送ろうとするとデッドロックを起こしてしまうため、非同期メッセージパッシングも一部で使われています。

## Hello Worldの一生

```mermaid
sequenceDiagram
participant hello as helloプログラム
participant user as userライブラリ
participant kernel as カーネル
participant uart as シリアルポート<br>(QEMU)
participant terminal as ホストOSの端末<br>(Terminal.appなど)

hello->>user: printf
user->>user: バッファリング
Note over user: 改行文字を受け取るか<br>バッファが一杯になるまで<br>バッファリング
user->>kernel: serial_writeシステムコール
kernel->>uart: シリアルポート出力
uart->>terminal: QEMUの標準出力
```

## タスクの一生

```mermaid
sequenceDiagram
participant shell as シェル
participant vm as VMサーバ
participant kernel as カーネル
participant new_task as 新しいタスク

shell->>vm: SPAWN_TASKメッセージ
vm->>kernel: task_createシステムコール
kernel->>+new_task: 実行開始
new_task->>-vm: ページフォルト<br>(PAGE_FAULTメッセージ)
vm->>kernel: ページをマップ<br>(vm_mapシステムコール)
vm->>+new_task: 処理を再開<br>(PAGE_FAULT_REPLYメッセージ)
new_task->>-vm: 正常・異常修了<br>(EXCEPTIONメッセージ)
vm->>kernel: タスクを削除<br>(task_destroyシステムコール)
vm->>shell: タスク終了メッセージ (TASK_DESTROYEDメッセージ)
```

## 同期的メッセージの一生

```mermaid
sequenceDiagram
participant sender as 送信側タスク
participant sender_lib as userライブラリ<br>(送信側タスク)
participant kernel as カーネル
participant receiver_lib as userライブラリ<br>(受信側タスク)
participant receiver as 受信側タスク

activate sender
activate receiver
sender->>sender_lib: メッセージ送信<br>(ipc_send API)
deactivate sender
activate sender_lib
sender_lib->>kernel: ipcシステムコール
deactivate sender_lib

receiver->>receiver_lib: メッセージ受信<br>(ipc_recv API)
deactivate receiver
activate receiver_lib
receiver_lib->>kernel: ipcシステムコール
deactivate receiver_lib

kernel->>receiver_lib: メッセージを受信
activate receiver_lib
receiver_lib->>receiver: メッセージを受信
deactivate receiver_lib
activate receiver

kernel->>sender_lib: 送信処理完了
activate sender_lib
sender_lib->>sender: 送信処理完了
deactivate sender_lib

deactivate receiver
```

## 非同期メッセージの一生

```mermaid
sequenceDiagram
participant sender as 送信側タスク
participant sender_lib as userライブラリ<br>(送信側タスク)
participant kernel as カーネル
participant receiver_lib as userライブラリ<br>(受信側タスク)
participant receiver as 受信側タスク

activate sender
activate receiver
sender->>sender_lib: メッセージ送信<br>(ipc_send_async API)
deactivate sender
activate sender_lib
sender_lib->>sender_lib: メッセージを<br>送信待ちリストに追加
sender_lib->>kernel: notifyシステムコール<br>(NOTIFY_ASYNC通知)
deactivate sender_lib

kernel->>sender_lib: 即座に処理を完了<br>(ノンブロッキング)
activate sender_lib
sender_lib->>sender: 送信処理完了
activate sender
deactivate sender_lib
Note over sender: 他の処理を進めていく

receiver->>receiver_lib: オープン受信<br>(ipc_recv API)
deactivate receiver
activate receiver_lib
receiver_lib->>kernel: ipcシステムコール
deactivate receiver_lib

kernel->>receiver_lib: NOTIFY_ASYNC通知を受信
activate receiver_lib

sender->>sender_lib: オープン受信<br>(ipc_recv API)
deactivate sender
activate sender_lib
sender_lib->>kernel: オープン受信<br>(ipcシステムコール)
deactivate sender_lib

receiver_lib->>sender_lib: 送信待ちメッセージ要求<br>(ASYNC_RECVメッセージ)
deactivate receiver_lib
activate sender_lib
sender_lib->>receiver_lib: 送信待ちメッセージを返信<br>(ipc_reply API)
activate receiver_lib
sender_lib->>kernel: オープン受信を継続<br>(ipcシステムコール)
deactivate sender_lib
receiver_lib->>receiver: メッセージを受信<br>(オープン受信)
deactivate receiver_lib
```

## パケットの一生

```mermaid
sequenceDiagram
participant shell as シェル<br>(HTTPクライアント)
participant tcpip as TCP/IPサーバ
participant vm as VMサーバ
participant driver as デバイスドライバサーバ
participant virtio as virtio-netデバイス

driver->>vm: net_deviceサービスとして登録
tcpip->>vm: net_deviceサービスを探索
vm->>tcpip: デバイスドライバサーバのタスクID
tcpip->>driver: ドライバの有効化・MACアドレスの取得<br>(NET_OPENメッセージ)
tcpip->>tcpip: DHCPによるIPアドレス取得
tcpip->>vm: tcpipサービスとして登録

shell->>vm: tcpipサービスを探索
vm->>shell: TCP/IPサーバのタスクID
shell->>tcpip: HTTPリクエスト<br>(TCP_WRITEメッセージ)
tcpip--)driver: パケット送信要求<br>(NET_SENDメッセージ・非同期)
driver->>virtio: 処理要求<br>(struct virtio_net_req)

virtio-->>driver: 受信パケット<br>(struct virtio_net_req)
driver->>tcpip: パケット受信通知<br>(NET_RECVメッセージ)
tcpip--)shell: データ受信通知<br>(TCPIP_DATAメッセージ・非同期)
shell->>tcpip: データ受信要求<br>(TCPIP_READメッセージ)
tcpip->>shell: HTTPレスポンス<br>(TCPIP_READ_REPLYメッセージ)
```
