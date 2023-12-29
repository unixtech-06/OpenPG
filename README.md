# OpenPageGetter

`OpenPageGetter` は、指定されたウェブページのHTMLコンテンツを取得し、標準出力に表示するC言語プログラムです。libcurlに依存することなく使うことができます。

## 機能

- 指定されたURLからHTMLコンテンツを取得します。
- HTMLコンテンツを標準出力に表示します。
- `-d` オプションを使用すると、コンテンツを `index.html` に保存します。

## 使用方法

プログラムをコンパイルするには、以下のコマンドを実行します：

```bash
mkdir build && cd build
cmake .. 
make
```

## プログラムを実行するには、以下のコマンドを使用します。

```bash
./pg example.com
```

## -d オプションを使用してHTMLをファイルに保存するには、以下のように実行します。

```bash
./pg -d example.com
```

## ライセンス

OpenPageGetter はBSD 3-Clause Licenseの下で公開されています。詳細はプロジェクトのソースファイルに記載されているライセンス文を参照してください。
