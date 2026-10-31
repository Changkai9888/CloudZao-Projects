# Plus 黑白字体组件

面向 **Metalio E-INK4 Plus 4.7英寸**，把已验收的 Noto Sans SC“稍实笔画”做成可直接复用的字库和小型C++组件。版本：1.0.0。

日常接入不需要自己生成字体、不需要下载TTF，也不需要Pillow。选择字号、字重，准备现成字库后编入自己的应用即可。本包同时提供轻量Skill和可复现生成工具。

## 一、快速接入

### 1. 先看交付内容

| 位置 | 用途 |
|---|---|
| `SKILL.md` | AI接入入口；只按需引导，不复制一大套项目规范 |
| `components/plus_font/` | 无堆内存分配、无SDK依赖的C++11字体读取与绘制模块 |
| `assets/fonts/` | 18套预生成稍实字体，主机端压缩存放 |
| `scripts/prepare_font.py` | 用Python标准库选取/裁剪现成字体，生成可编入应用的文件 |
| `scripts/generate_fonts.py` | 可选：从固定Noto字源重新栅格化或增加字号/字符 |
| `examples/render.cpp` | 可编译的最小接入与原生像素预览示例 |
| `tests/`、`验证报告.json` | 组件验证及交付证据；不等于每个新应用都已真机测试 |
| `LICENSE`、`licenses/` | 自有代码MIT、字体OFL，分别适用 |

整个目录可以移动或交接，工具通过自身位置找资源，不依赖作者电脑的其他项目。给支持SKILL.md的执行端安装时，按该执行端的安装方式放入**整个 `plus-noto-font` 目录**，不要只拷贝SKILL.md。也可以直接让AI阅读本目录的SKILL.md，或让真人按下文接入。这里不自动安装到全局执行端。

示例请求：使用 `$plus-noto-font`，为这个Plus应用接入32像素、中等、稍实字体；正文保留完整字库，菜单按文案裁剪，保持现有布局与刷新策略。

### 2. 选规格与字符覆盖

现成字号：**24、28、32、36、40、48像素**。

现成字重：**400常规、500中等、600半粗**。所有现成规格均为 **稍实112、1bpp黑白**。

每套包含7056个码点，其中6763个为GB2312汉字，其余为ASCII、常见中文/数学/货币符号、部分带声调字母等。精确清单以 `assets/fonts/characters.txt` 和清单JSON为准。它不是Unicode全字库，不保证生僻字、emoji或所有语言文字。

- 固定按钮、菜单：给工具UTF-8文案，裁剪成小字库。
- 记事本、浏览器等自由文本：保留完整选定规格，按业务需要补字符，不能只裁剪菜单字符。
- 一个应用可用多个规格，但每增加一个完整规格都会增加Flash占用。不要把测试矩阵全部编入应用。

18个压缩资源合计约8.59MiB，放在开发者电脑；**这不是每个固件必须增加的大小**。默认32px/500完整字库为995,783字节，实际进入应用的是选定字库、少量模块代码和许可文本。生成C++数组的文本文件比二进制大，不等于烧录体积也按文本大小计算。

### 3. 一条命令准备字体

进入本包目录，使用 **Python 3.10或更新版本**：

```sh
python scripts/prepare_font.py --size 32 --weight 500 --output output/body
```

准备一个只显示示例文案的轻量字体：

```sh
python scripts/prepare_font.py --size 32 --weight 500 --chars-file examples/text.txt --output output/demo
```

固定菜单文案以UTF-8存成自己的TXT替换示例即可。工具自动补ASCII，缺失字符会报错；不会悄悄丢弃请求的汉字。

生成目录包括：

- `font.h`、`font.cc`：直接编译，默认符号为 `plus_font_data`、`plus_font_data_size`、`plus_font_data_license`。
- `font.ntf`：同一字库的二进制，供自选资源嵌入方式使用；不是可烧录固件BIN。
- `font-manifest.json`：规格、字符、大小、来源SHA和结果SHA。
- `OFL.txt`、`FONT-NOTICE.txt`：原版权、完整OFL及衍生来源。

重复输出默认拒绝，确认更新的是自己的生成目录后可加 `--overwrite`。若采用自有二进制嵌入流程，加 `--binary-only`，但仍需自行把许可做成用户可访问的内容。

两个规格并存时分别输出到不同目录，并给第二套独立符号：

```sh
python scripts/prepare_font.py --size 40 --weight 600 --symbol-prefix title_font --output output/title
```

这里数组名就是 `title_font`，另外两个符号为 `title_font_size`、`title_font_license`，不会再附加 `_data`。两套头文件都叫font.h，应用可用限定路径 `#include "generated/body/font.h"` 和 `#include "generated/title/font.h"`，避免搜索路径误选；两个font.cc各编译一次，各自用一个Font对象引用。

### 4. 编入应用并绘制

ESP-IDF：复制 `components/plus_font` 到应用的 `components/plus_font`，把所选生成目录复制到应用 `main/generated/body`。

应用原有 `main/CMakeLists.txt` 的 `idf_component_register` 中，将 `generated/body/font.cc` 加入SRCS，将 `generated/body` 加入INCLUDE_DIRS，将 `plus_font` 加入REQUIRES；**保留原有条目**。不要另外重复调用一次注册。也不要同时编译font.cc又EMBED同一font.ntf。

其他C++构建：将模块的 `plus_font.cc`、生成的 `font.cc` 编译一次，并添加模块include及生成目录的头文件路径即可。

最小调用：

```cpp
#include "plus_font.h"
#include "font.h"
#include <cstring>

plus_font::Font body;
// 应用初始化时执行一次；font数据必须保持有效且不可修改。
bool ok = body.Open(plus_font_data, plus_font_data_size);

// 示例：横屏1216×684，1=黑。实际参数必须匹配自己的缓冲区。
// frame至少152*684字节，并由应用负责填白、擦旧字及刷新。
plus_font::Surface1bpp surface{frame, frame_bytes, 1216, 684, 152, true};
const char* text = u8"中文清晰 Aa09，稍实笔画。";
if (ok) {
    const int baseline = 24 + body.metrics().ascent;
    auto result = plus_font::DrawText(body, surface, text, std::strlen(text), 24, baseline);
    // 检查result.status、missing_glyphs和invalid_utf8，再走应用原有显示提交。
}
```

上段展示调用位置，完整可编译程序见 `examples/render.cpp`。字符必须来自所选字库；示例文案文件用于演示裁剪。

**竖屏684像素宽时，每行需要86字节，不是85.5字节；横屏1216宽为152字节。** Surface按行补齐，字体内部字形却按连续位打包，两者不可混淆。

Surface的 `one_is_black` 只表示你的内存缓冲极性，不假定面板协议也一样。若已有显示层使用灰度缓冲、旋转布局或专有编码，应在绘制适配层转换；不要把NTF字形位图直接当EPDiy帧缓冲。

### 5. 几个接口足够使用

| 接口 | 作用 |
|---|---|
| `Font.Open(data, size)` | 校验并引用一份字库；每份字库只需初始化一次 |
| `Font.Find(codepoint, &glyph)` / `Contains` | 精确取字、检查覆盖；可用于接入LVGL等已有GUI |
| `spec()` / `metrics()` | 获取字号字重阈值与基线行高 |
| `MeasureText(...)` | 用实际advance测量，不用另写一套估算宽度 |
| `DrawText(...)` | 原生像素黑白绘制，自动裁切到Surface边界 |

没有运行时缩放、动态字体生成、自动换行、复杂文字塑形或字偶距；主要面向中文、普通英文/数字和已列符号。自动换行、对齐和省略号由应用用相同advance完成。

LF、CR、CRLF换行；TAB前进四个空格宽度，不对齐制表位。缺字绘制问号并增加 `missing_glyphs`，非法UTF-8逐非法字节替代并计数。返回 `width` 是行的前进宽度，不是墨迹包围盒。所有接口以显式UTF-8字节长度为边界。

绘制是透明的，只修改字形墨点；改短文字前先擦原区域，否则会残留旧字。最后参数 `ink_black=false` 可在已填黑背景上写白字。

模块只写内存，不操作触摸、Wi-Fi、电源、存档、固件口袋协议或屏幕刷新。首屏全刷、日常DU及研墨继续使用应用已验证的显示链。

### 6. 许可必须随接入保留

新增组件、工具代码和文档按用户明确选定的MIT授权，版权行采用项目贡献者署名，见根 `LICENSE`。Noto原字体及本包衍生位图另按OFL1.1，保留Adobe原版权及完整许可，见 `licenses/`。不能用MIT覆盖字体许可，也不能把Noto字形署名为自己原创。

**这里的“根LICENSE”指本子项目 `plus-noto-font/LICENSE`。其MIT授权仅适用于 `CloudZao-Projects/plus-noto-font/` 内的原创文件，不适用于整个CloudZao-Projects仓库、父目录或其他子项目。** 本项目以独立子目录发布，不在仓库根目录放置或更改许可证。

发布前已核对固定字源、原版权与OFL完整性、衍生字体命名、代码/字体许可范围及发布文件清单，未发现阻碍本次发布的许可问题。这一核查不代表对未知第三方权利主张的保证。软件工具无需把Pillow、fontTools或原TTF随本项目重新分发，扩展生成时按各自许可使用这些外部工具。

准备脚本将原版权及OFL作为 `plus_font_data_license` 字符串生成到font.cc。应用应通过帮助/关于/许可页实际引用并显示完整内容，或提供其他用户可方便查看的完整许可途径；仅把未被引用的字符串放进源码，可能被链接器丢掉，不能当作已经完成展示。若使用自定义prefix，对应名称为 `<prefix>_license`。

随源码或资源分发时保留根LICENSE及对应OFL/FONT-NOTICE。只有BIN的发布渠道也应让用户在固件中查到字体许可；涉及应用其他第三方驱动的义务仍由应用自己处理，本组件不替整机做合规认证。

## 二、理解与复现

### 为什么是这种字体效果

这里使用原生像素栅格化：同一个Noto源字体先选择真实wght轴，再以目标像素字号生成覆盖率遮罩。覆盖率从0到255，**大于等于112写墨点**，否则留白。

相比测试对照阈值128，112保留更多边缘像素，因此叫稍实。它与字重独立：400、500、600分别是常规、中等、半粗；不额外膨胀或描边。最终字形仍只有黑与白，不显示灰色抗锯齿边缘。

统一基线，并保留每个字的天然宽高、bearing和advance，是避免“有大有小、七扭八歪”的关键。不要把Noto逐字挤进另一套字体的字框，也不要把每个字的墨迹包围框拉成相同高度。

### 如何精确重生成或扩展

只在现成规格/字符不够，或研究生成方法时走这条路。日常prepare无需以下依赖。

1. 从[固定Noto源版本](https://raw.githubusercontent.com/google/fonts/2894aab31764f10f29c421bdfd2340d3b382d384/ofl/notosanssc/NotoSansSC%5Bwght%5D.ttf)取得变量字体。
2. 源SHA256须为 `a3041811a78c361b1de50f953c805e0244951c21c5bd412f7232ef0d899af0da`；脚本会检查，不自动下载或切换版本。
3. 复用已有Python环境，按 `scripts/generator-requirements.txt` 准备Pillow和fontTools。本次光栅器为Pillow13.0.0 / FreeType2.14.3 / fontTools4.61.1。精确工具记录在资源清单中；跨平台FreeType版本不同需比对输出，不承诺字节自动一致。
4. 输出到新目录，保护现有已交付资源：

```sh
python scripts/generate_fonts.py --source /path/to/NotoSansSC-wght.ttf --output rebuilt-fonts
```

可加 `--sizes 32,40 --weights 500,600 --chars-file your-text.txt`。生成器支持16～64px及三种已定义字重，但六种既有字号以外的尺寸属于新候选，需自行检查目标页面，不冒充已实机验收。

5. 再用现成准备流程输出：

```sh
python scripts/prepare_font.py --assets rebuilt-fonts --size 32 --weight 500 --output output/rebuilt
```

渲染规则：`getmask2(..., mode='L', anchor='ls')`取得字形及相对基线偏移；advance按源测量值四舍五入到整数。对于24/28/32/36/40/48px，保留已验收测试的行度量基线；24/48px的descent兼顾原测试用到的备用字体度量，但所有字形仍只来自Noto Sans SC。若新增字符越过度量，生成器只扩大必要的行界。

### 资源格式和轻量化

NTF1使用40字节头、24字节规格、20字节字形记录；本组件每包仅一个规格，不包含旧测试专用汉字目录。字形按Unicode递增，二分查找。每个字包含码点、位图偏移、宽高、bearing和advance；MSB优先，1为墨点，每个字形位图末尾补齐字节，字形内部各行连续。

外层zlib只用于开发包运输。prepare在电脑上解压，设备不需要zlib、Pillow、fontTools或TTF解析器。Font对象仅引用只读数据；静态数组可留在Flash映射区，不必把整套字库复制到RAM。

二进制格式细节可读 `scripts/font_format.py` 与组件源码；它是组件内部资源格式，不是固件口袋应用接入协议，不改变应用13MiB等分区上限。

### 如何核对没有变形或漏字

从最终NTF数据反解，而不只看生成前的源图；核对位图、bearing、advance、行高及字符覆盖。用真实DrawText生成原生预览，检查中文、英文、数字、标点、边界、换行及黑底白字，再在目标设备确认。

本轮源样本已有用户实机验收，明确稍实优先。公共组件补齐的18套7056码点属于本轮生成与主机验证成果，不能说127,008份字形均由用户逐字实测。移植到新应用后仍需核对实际布局。

### 最小主机预览

Linux或WSL已有g++时，在包目录执行：

```sh
python3 scripts/prepare_font.py --size 32 --weight 500 --chars-file examples/text.txt --output output/demo
g++ -std=c++11 -O2 -Wall -Wextra -Werror -Icomponents/plus_font/include -Ioutput/demo components/plus_font/plus_font.cc output/demo/font.cc examples/render.cpp -o output/font-demo
./output/font-demo output/demo.pbm
```

这是主机预览程序，不烧录、不操作设备。PBM是原生1216×684纯黑白图，查看时按100%像素检查；需要PNG可用现有图像工具转换。若输出目录已生成，复用现有文件或明确使用overwrite，不必重复生成。

已生成的原生预览见 `examples/preview.png`；独立交接试用与旧样本逐字比对的精简证据保存在 `tests/evidence/`。临时编译产物不随交接包分发。

完整主机检查可运行 `bash tests/run_host.sh`；含18套字体、独立小字形边界fixture、ASan/UBSan与可编译示例。普通CMake接入可用 `cmake -S tests -B build -DCMAKE_BUILD_TYPE=Debug`，再构建并运行ctest。已有ESP RISC-V工具链时，可设置TARGET_CXX后运行 `tests/compile_target.sh` 做对象编译检查；这些命令都不烧录设备。

交接给别人时使用 `交付/plus-noto-font-1.0.0.zip`，解压后整个目录即可使用。需要重新打包可执行 `python scripts/package_handoff.py`；只收入组件、资源、说明与验证报告，不收入本机测试缓存，包内有逐文件SHA256清单。

详细验证结果和适用边界见根 `验证报告.json`。这份组件的交付不等于已经给固件口袋、记事本或其他冻结应用换字库。
