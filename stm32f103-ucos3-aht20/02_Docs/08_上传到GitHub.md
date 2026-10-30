# 08 · 把这两个工程推到 GitHub

> 面向本机现状写的操作手册。每一步都给**可以直接复制**的命令，以及出错时的对照表。
> 本机实测信息（2026-10-30）：
>
> - `git version 2.56.0.windows.2` 已安装，`gh`（GitHub CLI）**没装**
> - `github.com:443` / `api.github.com` / `ssh.github.com:443` **全部连不上**（`curl` 返回 `000`，
>   DNS 正常解析到 `20.205.243.166`）；而 `codeload.github.com`（301）、`gitee.com`（200）**是通的**
> - 系统里没有 `HTTP_PROXY` / `HTTPS_PROXY` 环境变量，`127.0.0.1` 的 7890 / 7897 / 10809 / 1080 / 8080 都没有代理在监听
>
> 结论：**代码已经整理成仓库了，只差"联网 + 认证"这一步**。想直接推成功，先开代理/VPN，再看第 4 节。

---

## 1. 现在到哪一步了

| 事项 | 状态 |
| --- | --- |
| 建立本地 git 仓库（分支 `main`） | ✅ 已完成 |
| 写 `.gitignore`（排除 Keil 编译产物、60 MB 用不到的 CMSIS DSP） | ✅ 已完成 |
| 首次提交（661 个文件，38.8 MB，提交说明已写好） | ✅ 已完成 |
| 补 `README.md`（仓库首页）、`THIRD_PARTY_NOTICES.md`、`LICENSES/Apache-2.0.txt` | ✅ 已完成 |
| 在 GitHub 上建空仓库 | ⬜ 你要做（第 2 节） |
| 把提交作者改成你自己的名字/邮箱 | ⬜ 你要做（第 3 节） |
| 配代理、关联远程、`git push` | ⬜ 你要做（第 4、5 节） |

---

## 2. 第一步：在 GitHub 上建一个**空**仓库

1. 打开 <https://github.com/new>
2. **Repository name**：建议 `stm32f103-ucos3-aht20`（或 `STM32-RTOS-AHT20`，别用中文名）
3. **Description**：`STM32F103C8T6 + uC/OS-III 移植 + AHT20 温湿度采集（Keil MDK / HAL）`
4. 可见性：`Public`（作业展示）或 `Private` 都行
5. **下面三个勾全部不要勾**：不要 `Add a README file`、不要 `.gitignore`、不要 `license`
   —— 本地已经有 README 和 .gitignore 了，勾了会在推送时冲突，需要多一步 `pull --rebase`
6. 点 `Create repository`，页面会显示一段命令，**只看 `git remote add origin ...` 这一行**即可

拿到你的仓库地址，形如：

```text
https://github.com/<用户名>/<仓库名>.git
```

---

## 3. 第二步：把提交作者改成你自己

现在那个提交用的是占位身份（`ZhuanZ <zhuanz@users.noreply.github.com>`），
把下面的名字/邮箱换成**你 GitHub 账号里的**（邮箱必须是你账号已验证的邮箱，
否则 GitHub 不会把提交算到你头上、贡献图也不会变绿）：

```bash
cd D:/dsh_list/STM32F103_RTOS_AHT20

git config user.name  "你的名字"
git config user.email "你的GitHub邮箱@example.com"

# 把已有的那次提交改成新身份（只改作者信息，不改内容）
git commit --amend --reset-author --no-edit
```

想顺便让终端里的中文文件名正常显示（默认会显示成 `\345\215\232...`）：

```bash
git config --global core.quotepath false
```

---

## 4. 第三步：先把网络打通（这一步绕不过去）

`github.com:443` 现在是**连不上**的，`git push` 一定会卡在：

```text
fatal: unable to access 'https://github.com/xxx/yyy.git/':
Failed to connect to github.com port 443 after 21093 ms: Could not connect to server
```

这行报错**不是 git 配置问题，是网络问题**。三种办法，按推荐顺序：

### 方案 A（推荐）：开代理/VPN，让 git 也走代理

先打开你的代理软件（Clash / v2rayN / 加速器…），确认它的**本地端口**：

| 软件 | 常见 HTTP 端口 |
| --- | --- |
| Clash / Clash Verge | `7890` |
| Clash Verge（新版混合端口） | `7897` |
| v2rayN | `10809`（HTTP）、`10808`（SOCKS） |
| Shadowsocks | `1080`（SOCKS5） |

先手动验证代理确实能用：

```bash
# 把 7890 换成你自己的端口；返回 200 就说明通了
curl -x http://127.0.0.1:7890 -s -o /dev/null -w "%{http_code}\n" https://github.com
```

返回 `200` 之后，给 git 配上（**注意是 `http.proxy`，HTTPS 也用它**）：

```bash
git config --global http.proxy  http://127.0.0.1:7890
git config --global https.proxy http://127.0.0.1:7890

# 验证
git ls-remote https://github.com/<用户名>/<仓库名>.git
```

以后想取消（比如换网络后不需要代理了）：

```bash
git config --global --unset http.proxy
git config --global --unset https.proxy
```

### 方案 B：只给这一次 push 用代理（不改全局配置）

```bash
git -c http.proxy=http://127.0.0.1:7890 push -u origin main
```

### 方案 C：完全没有代理/VPN 时的兜底

`gitee.com` 是通的，可以"曲线入库"：

```bash
# 1) 先推到 Gitee（提前在 gitee.com 建好同名空仓库）
git remote add gitee https://gitee.com/<你的Gitee名>/<仓库名>.git
git push -u gitee main

# 2) 打开 https://github.com/new/import
#    Repository URL 填 Gitee 的 HTTPS 地址，GitHub 服务器会替你把代码拉过去
#    （这一步不需要你本机能连 GitHub，只要 Gitee 仓库是 Public）
```

或者最简单粗暴：本地打包后从 GitHub 网页 `Add file → Upload files` 手动传
（先 `git archive -o repo.zip HEAD` 生成 zip，注意解压后是源码而不是 `.git`）。

---

## 5. 第四步：关联远程仓库并推送

```bash
cd D:/dsh_list/STM32F103_RTOS_AHT20

# 关联（地址换成你自己的）
git remote add origin https://github.com/<用户名>/<仓库名>.git

# 把本地 main 推上去，并建立跟踪关系
git push -u origin main
```

### 关于认证

- **HTTPS 方式**：GitHub 从 2021 年起**不再接受账号密码**。`git push` 时 Windows 会弹出
  「Git Credential Manager」窗口 → 选 `Sign in with your browser` → 浏览器登录授权即可，
  之后会记住凭据，不用每次输。
- 如果弹出的是命令行让你输入用户名/密码，密码处要填 **Personal Access Token**：
  <https://github.com/settings/tokens> → `Generate new token (classic)` → 勾 `repo` → 生成后复制
  （只显示一次）。用户名填你的 GitHub 用户名，密码填这个 token。
- **SSH 方式**（可选，需先在本地生成密钥）：

  ```bash
  ssh-keygen -t ed25519 -C "你的邮箱"          # 一路回车
  cat ~/.ssh/id_ed25519.pub                    # 复制输出
  # 粘贴到 https://github.com/settings/keys → New SSH key
  ssh -T git@github.com                        # 出现 "Hi xxx! You've successfully authenticated"
  git remote set-url origin git@github.com:<用户名>/<仓库名>.git
  ```

  如果 `22` 端口被墙，用 443 端口：

  ```bash
  # ~/.ssh/config 里加：
  Host github.com
    HostName ssh.github.com
    Port 443
    User git
  ```

  注意：本机实测 `ssh.github.com:443` 现在也不通，开代理后才可用，且需要给 SSH 单独配代理
  （`ProxyCommand connect -H 127.0.0.1:7890 %h %p` 之类的包装器），比 HTTPS 麻烦，
  所以**优先走 HTTPS + 浏览器登录**。

---

## 6. 常见报错对照表

| 报错/现象 | 原因 | 处理 |
| --- | --- | --- |
| `Failed to connect to github.com port 443 after ... ms` | 网络被墙 | 见第 4 节，开代理后给 git 配 `http.proxy` |
| `fatal: unable to access ...: SSL certificate problem` | 代理软件做了中间人证书 | `git config --global http.sslVerify false`（仅临时） |
| `remote: Support for password authentication was removed` | 用了账号密码 | 改用 Personal Access Token 或浏览器登录 |
| `remote: Repository not found` | 地址错 / 仓库没建 / token 无 `repo` 权限 | 核对 URL；重新生成带 `repo` 权限的 token |
| `error: src refspec main does not match any` | 本地还没有提交，或分支叫 `master` | `git log` 确认有提交；`git branch -M main` |
| `! [rejected] main -> main (fetch first)` | 远程仓库不是空的（建仓库时勾了 README） | `git pull --rebase origin main` 后再 push |
| `! [rejected] ... (non-fast-forward)` | 远程有你本地没有的提交 | 同上，先 `pull --rebase` |
| `error: remote origin already exists` | 重复 `remote add` | `git remote set-url origin <新地址>` |
| `GH001: Large files detected` / `this exceeds GitHub's file size limit of 100.00 MB` | 提交了编译产物或 `.lib` | 本项目 `.gitignore` 已挡住；若已误提交，先 `git rm --cached <文件>` 再 `commit --amend` |
| push 卡住不动、没报错 | 代理没生效 / 大文件上传慢 | 加 `GIT_TRACE=1 GIT_CURL_VERBOSE=1 git push` 看卡在哪一步 |
| 网页上中文文件名/文档乱码 | 编码 | 源码和文档都是 UTF-8，正常显示；终端乱码只是 `core.quotepath` 的事，见第 3 节 |

---

## 7. 推上去之后

**每次改完代码只要三条命令**：

```bash
git add -A
git commit -m "fix: 修好 TASK3 的串口输出"
git push
```

其它可选动作：

- **把 hex 也发布出来**（仓库里默认不含编译产物）：
  `Releases → Draft a new release → 选 tag v1.0 → 上传 MDK-ARM/AHT20_uCOS3/AHT20_uCOS3.hex`
  → 别人不用装 Keil 就能直接烧板子验证。
- **仓库首页美化**：`Settings → General → Topics` 加
  `stm32f103`、`ucos-iii`、`rtos`、`aht20`、`i2c`、`keil-mdk`、`hal`；右侧 About 填一句简介。
- **确认许可**：`RTOS/` 里是 Apache-2.0 的 uC/OS-III，`THIRD_PARTY_NOTICES.md` 已经写明；
  如果要给自家代码加许可证，直接加个 `LICENSE`（MIT 最省事）。

## 8. 仓库为什么是 38.8 MB

| 内容 | 大小 | 说明 |
| --- | --- | --- |
| `Drivers/STM32F1xx_HAL_Driver/`（两份，每工程一份） | 各 5.8 MB | HAL 全量驱动，编译只用到其中 13 个 `.c` |
| `Drivers/CMSIS/Device/ST/STM32F1xx/Include/` | 各约 9 MB | 各种 F1 型号的头文件，实际只用 `stm32f103xb.h` |
| `RTOS/`（uC-OS3 + uC-CPU + uC-LIB） | 1.7 MB | 内核源码，必须保留（否则工程编不过） |
| `02_Docs/` | 0.56 MB | 7 篇文档 + 2 张接线图 |

已经排除掉的和想进一步瘦身的：

- 已排除：`Drivers/CMSIS/DSP/`（535 个文件）、`Drivers/CMSIS/Lib/*.lib`（13 MB 的数学库）、
  `MDK-ARM/<工程名>/`（Keil 输出目录）、`*.uvguix.*`、`*.log`、`dis*.txt`
- 还想更小可以只留 `stm32f103xb.h`，一个工程能再省约 9 MB：

  ```gitignore
  **/Drivers/CMSIS/Device/ST/STM32F1xx/Include/stm32f1*.h
  !**/Drivers/CMSIS/Device/ST/STM32F1xx/Include/stm32f103xb.h
  ```

  （加之前先确认你的 `.uvprojx` 里 `<Device>STM32F103C8</Device>`、宏 `STM32F103xB` 没变）

---

## 9. 推到**已有**仓库：`GaLa322/GaLa_s_git`

你已经有仓库了（默认分支 `main`），里面现在是一堆按实验平铺的成果：

```text
GaLa_s_git/
├── .gitkeep
├── LED_Flow.zip
├── PWMBreath.uvprojx
├── Task.zip
├── TimerLED.uvprojx
└── main.c
```

所以**不要**把本仓库直接 push 上去（两边历史互不相干，会冲突），
正确做法是把它当成**一个子目录**加进去，原有文件一个都不动。

### 方法一：双击脚本（推荐）

脚本 [`tools/push_to_repo.bat`](tools/push_to_repo.bat)（纯 ASCII + CRLF，已在一个本地测试仓库上
完整跑通：克隆 → 导出 664 个文件 → 提交 → 推送，`exit 0`）

1. 先开代理 / VPN
2. 双击 `02_Docs\tools\push_to_repo.bat`
3. 脚本依次做：检查 github 连通性 → 把你的仓库克隆到 `D:\dsh_list\_push_work\GaLa_s_git`
   → 把本工程导出到 `GaLa_s_git\stm32f103-ucos3-aht20\` → 提交 → `git push`
4. 想推到别的仓库：`push_to_repo.bat https://github.com/you/your-repo.git`

推送完成后你的仓库长这样：

```text
GaLa_s_git/
├── .gitkeep  LED_Flow.zip  PWMBreath.uvprojx  Task.zip  TimerLED.uvprojx  main.c   ← 原样保留
└── stm32f103-ucos3-aht20/                  ← 新增（664 个文件，无任何编译产物）
    ├── README.md
    ├── 01_Project/AHT20_RTOS/  AHT20_uCOS3/
    └── 02_Docs/
```

### 方法二：手动 6 条命令（Git Bash）

```bash
cd /d/dsh_list
git clone https://github.com/GaLa322/GaLa_s_git.git _GaLa
cd _GaLa
git clone /d/dsh_list/STM32F103_RTOS_AHT20 stm32f103-ucos3-aht20   # 本地克隆 = 只带已提交的文件
rm -rf stm32f103-ucos3-aht20/.git                                   # 去掉内层仓库
git add -A && git commit -m "feat: 添加 STM32F103 uC/OS-III 移植 + AHT20 温湿度采集工程"
git push
```

> PowerShell 里 `rm -rf` 换成 `Remove-Item -Recurse -Force stm32f103-ucos3-aht20\.git`。

### 注意事项

- **历史不保留**：这是"搬文件"。子目录里的 3 个提交属于本仓库，不会跟过去，
  你的仓库只会多出 **1 个提交**（`feat: 添加 …`）。
- 子目录自带 `.gitignore`，`MDK-ARM/<工程名>/` 里 143 MB 编译产物不会进仓库；
  实测子目录是 **664 个文件 / 43 MB 工作区 / 约 23 MB git 对象**。
- **别用 `tar` 或 `git archive` 搬**：Windows 自带 `tar.exe` 处理中文文件名会报
  `Invalid empty pathname`，用本地 `git clone` 才稳（脚本里就是这么做的）。
- 想让工程出现在仓库首页，就在根目录加个 `README.md`，里面写
  `- [STM32F103 uC/OS-III + AHT20](stm32f103-ucos3-aht20/README.md)`。
