# SilexVK (WIP)

レンダリングの学習用に使用している Vulkan レンダラーです。<br>



## 環境

* Windows 10 / 11
* MSVC - C++ 20
* Vulkan SDK 1.2
* NVIDIA GeForce GTX 1060



## ビルド

```bat
git clone --recursive https://github.com/Suuta/Silex.git
```

クローン後に ***project.bat*** を実行して *VisualStudio* ソリューションを生成します。<br>
生成後、ソリューションを開いてビルドをするか ***build.bat*** の実行でビルドが行われます。<br>
プロジェクト生成ツールに [Premake](https://premake.github.io/) を使用していますが、別途インストールは必要ありません。<br>



## 操作

| 操作         | バインド                      |
| ------------ | ----------------------------- |
| カメラ移動   | 右クリック + W, A, S, D, Q, E |
| シーンを開く | Ctr + O                       |
| シーンを保存 | Ctr + S                       |
| 新規シーン   | Ctr + N                       |



