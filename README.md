# ToonShader 卡通渲染项目整理

本仓库用于整理卡通渲染相关工程文件和修改过的源码快照。为了避免不同工程内容互相混杂，UE、Unity、Blender 以及改过 UE 源码后的 Test 项目分别放在不同分支。

## 分支说明

| 分支 | 内容 | 说明 |
| --- | --- | --- |
| `main` | 改过 UE 源码后的 Test 项目 | 包含 `Test.uproject`、项目 `Source`、必要 `Config`、关卡与角色资源，以及 `Deliverables/ModifiedCodeFiles_20260616_0028` 下的修改源码快照。 |
| `ue-toonshader` | 未改 UE 源码的 UE 端 ToonShader 项目 | 来自本地 `E:\Projects\UE\ToonShader`，只上传项目必需文件，未包含缓存、插件、IDE 文件和中间产物。 |
| `unity-toonshader` | Unity 端 ToonShader 项目 | 来自本地 `E:\Projects\Unity\ToonShader`，包含 `Assets`、`Packages`、`ProjectSettings`，未包含 `Library`、`Logs`、`obj`、`UserSettings` 等生成目录。 |
| `blender-miyabi-goo41` | Blender 文件 | 只包含 `Miyabi_Goo41.blend`。 |

## main 分支内容

`main` 分支是改过 UE 源码后的展示工程。由于完整 UE 源码体积很大，本仓库没有直接上传完整引擎源码，只在 `Deliverables/ModifiedCodeFiles_20260616_0028` 中保留了本项目相关的修改源码快照。

该分支保留的主要内容：

- `Source/Test`：项目 C++ 代码。
- `Config`：项目配置。
- `Content/NewMap1.umap`：展示关卡。
- `Content/TestToon/Miyabi`：卡通渲染材质与实例。
- `Content/ZZZ/Miyabi`：角色模型、材质与贴图资源。
- `Deliverables/ModifiedCodeFiles_20260616_0028`：UE 引擎侧和项目侧改动源码快照，只保留代码文件。

未上传的内容包括本地缓存、中间产物、IDE 文件、辅助脚本、插件、笔试要求文档、压缩包、视频和其他非必要交付文件。

## 使用方式

克隆仓库后建议先安装 Git LFS：

```bash
git lfs install
git clone https://github.com/luolaniii/ToonShader.git
cd ToonShader
git lfs pull
```

切换不同分支查看对应工程：

```bash
git checkout main
git checkout ue-toonshader
git checkout unity-toonshader
git checkout blender-miyabi-goo41
```

## 注意事项

- UE 相关分支中的 `.uasset`、`.umap` 等二进制资源使用 Git LFS 管理。
- Unity 分支中的模型和贴图资源也使用 Git LFS 管理。
- Blender 分支中的 `.blend` 文件使用 Git LFS 管理。
- `main` 分支需要配合本地已修改过的 UE 源码环境使用；完整 UE 源码不在本仓库内。
