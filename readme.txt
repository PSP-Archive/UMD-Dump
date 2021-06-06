umd_dump.prx ver 1.3

概要
XMB/GAME上で VIDEO/AUDIO ISO / GAME ISO / 体験版ISO の吸出しを行います
他の吸出し方法では出来ない、UMD VIDEOの最終セクタまでの吸出しが可能です

※実行環境と吸出せるデータの一覧

             ↓環境 →入れているUMD | UMD GAME  | UMD VIDEO/AUDIO | UMD VIDEO/AUDIO + GAME |
------------------------------------+-----------+-----------------+------------------------+
                                XMB | GAME ISO  | VIDEO/AUDIO ISO |    VIDEO/AUDIO ISO     |
                               GAME | GAME ISO  |        ?        |        GAME ISO        |
              体験版EBOOT.PBP実行中 | 体験版ISO |    体験版ISO    |       体験版ISO        |
XMBからumd_dump経由でiso_toolを実行 | GAME ISO  | UPDATE部分 ISO  |        GAME ISO        |

動作環境
PSP-2000/CFW 5.50 GEN-D3 で動作確認

インストール
/seplugins/にumd_dump.prx　と　umd_dump.ini/をコピーします
/seplugins/のVSH.TXT/GAME.TXTにms0:/seplugins/umd_dump.prx 1を追記し、再起動させます
(同梱のVSH.txtを参考にしてください)
リカバリーモードのPluginsにて、umd_dump.prxがEnableになっているのを確認します

使い方
起動ボタン（標準でVSHではHOME,GAMEでは♪）でUMDの吸出し確認画面が表示されます
○で吸出し開始、×でキャンセルです
保存場所はUMD VIDEO/UMD AUDIOは/iso/video/、UMD GAMEは/iso/にUMD IDで保存されます
吸出中に×でキャンセルすることが出来ます
終了後は○でリブートします

起動後に□ボタンで、指定したEBOOT.PBPを起動します
この状態だとUPDATE部分のセッションの吸出しができます

XMB/GAMEの動作を止めている関係上、最後はどうしてもリブートが必要となりました

注意事項
エラー等により通常終了できなかった場合は、
「01234565789012345678901234567890123456789012345678901234567890123456789」
というファイルが、/iso/video/、または/iso/に保存されますので、削除してください
※不完全なファイルがXMBで認識されないようなファイル名を内部で利用しています

***********************************************************************
＊カスタマイズについて
・umd_dump.iniにて指定します

 1行目にVSHでの起動キーを指定します。 標準は「HOME」です
 2行目にGAMEでの起動キーを指定します。標準では「♪」です

 以下のキーを | で区切って指定できます

 PSP_CTRL_CIRCLE / PSP_CTRL_CROSS / PSP_CTRL_SQUARE / PSP_CTRL_TRIANGLE
 PSP_CTRL_UP / PSP_CTRL_DOWN / PSP_CTRL_RIGHT / PSP_CTRL_LEFT
 PSP_CTRL_RTRIGGER /  PSP_CTRL_LTRIGGER
 PSP_CTRL_VOLUP /  PSP_CTRL_VOLDOWN
 PSP_CTRL_SCREEN /  PSP_CTRL_NOTE /  PSP_CTRL_SELECT /  PSP_CTRL_START
 PSP_CTRL_DISC / PSP_CTRL_HOLD / PSP_CTRL_WLAN_UP

例：PSP_CTRL_START | PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER

 3行目には□ボタンで起動するEBOOT.PBPのパスを指定します
 指定しない場合は"ms0:/psp/game/iso_tool/"が設定されます

***********************************************************************
takka@tfact.net


***********************************************************************
開発履歴

Ver 1.31
[BUG] メニュー表示が確実に行われないのを修正

Ver 1.3
[UPDATE] GAMEでの起動ができるように、バッファを動的に確保する様に調整
[UPDATE] VSHとGAMEでの起動キーをそれぞれ設定できるように変更
         ※umd_dump.iniのフォーマットが変更されているので注意してください

Ver 1.2
[UPDATE] UMD VIDEO/AUDIOの判別をUMD_DATA.BINを利用するように変更
[NEW] UMD_DATA.BINの書換え機能を追加
[NEW] UPDATE部分の読込みの為、外部ソフトの起動機能を追加
[UPDATE] 上記に伴ない、設定ファイルの変更
[UPDATE] 起動時のエラーチェックを追加

Ver 1.0
[NEW] 吸出中は自動パワーオフ機能を無効にするようにした
[NEW] 吸出後には再起動が必要になりました
[NEW] 実行中止を追加
[NEW] 吸出し状況の表示を追加
[NEW] 吸出し前に確認を追加
[UPDATE] 保存場所をUMD VIDEO/UMD AUDIOは/iso/video/に、UMD GAMEは/iso/に変更
[UPDATE] UMD IDで保存するように変更
[UPDATE] 起動ボタンをHOMEに変更
[BUG] video.isoのサイズが大きいままになるBUGを修正

Ver. test
[NEW] UMD VIDEOの吸出しを実装
