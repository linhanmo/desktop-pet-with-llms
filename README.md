# XiaoMo 妗岄潰瀹犵墿锛堢绾挎湰鍦?LLM + 璇煶鍚堟垚锛?
涓€娆惧熀浜?Qt + Live2D 鐨勬闈㈠疇鐗╁簲鐢紝鏀寔鏈湴绂荤嚎 LLM 瀵硅瘽涓庣绾?TTS锛堟枃瀛楄浆璇煶锛夛紝鎻愪緵鎵樼洏鑿滃崟涓庡叏灞€蹇嵎閿紝瑙ｅ帇鍗崇敤鎴栦粠婧愮爜鏋勫缓銆?

## 鍔熻兘姒傝
- Live2D 鐪嬫澘濞橈細鏀寔鍛煎惛銆佺湪鐪笺€佽绾胯窡闅忋€佺墿鐞嗘ā鎷燂紙澶村彂/琛ｇ墿鎽嗗姩锛夛紝鍙缃睆骞曟樉绀恒€侀€忔槑鑳屾櫙銆佸叏灞€缃《绛夈€?- 鏈湴绂荤嚎 LLM 瀵硅瘽锛氬唴缃?llama.cpp CLI 璋冪敤锛屾敮鎸?1.5B/7B 瑙勬ā锛岄鏍煎彲閫?Original/Universal/Anime锛孲ystem Prompt 鍙嚜瀹氫箟銆?- 绂荤嚎 TTS 鎾姤锛氶泦鎴?sherpa-onnx 鐢熸€佺殑澶氱绂荤嚎璇煶妯″瀷锛屾敮鎸佸璇磋瘽浜猴紙sid锛夈€侀煶閲忚皟鑺傘€?- 瀵硅瘽涓庢皵娉★細瑙掕壊鏃佹樉绀哄璇濇皵娉★紝鏀寔涓嶅悓姘旀场涓婚锛涜亰澶╃獥鍙ｆ敮鎸佹竻绌哄巻鍙层€佷笂涓嬫枃鏉℃暟涓庢渶澶х敓鎴愰暱搴﹀彲璋冦€?- 鏈湴鎻愰啋锛氬唴缃畾鏃朵换鍔★紙姣忓ぉ鍥哄畾鏃堕棿鎴栨寜闂撮殧锛夊脊鍑烘彁绀哄苟鍙┍鍔ㄥ姩浣滅粍銆?- 澶氳瑷€涓庝富棰橈細鏀寔绠€浣撲腑鏂囥€佽嫳鏂囦笌涓婚鍒囨崲锛涢厤缃笌瀵硅瘽鍘嗗彶鎸佷箙鍖栧瓨鍌ㄣ€?

## 鐜瑕佹眰锛堣繍琛岋級
- Windows 10/11锛坸64锛夈€?- 寤鸿瀹夎 7-Zip锛堢敤浜庤В鍘?build-msvc 鍒嗗嵎锛夈€?- 濡傞渶鏈湴 LLM / 绂荤嚎璇煶锛岃涓嬭浇瀵瑰簲璧勬簮鍖呭苟瑙ｅ帇鍒扮▼搴忕洰褰曠殑 `res/` 涓嬶紙缁撴瀯瑙佷笅鏂団€滆祫婧愪笌鐩綍绾﹀畾鈥濓級銆?

## 鐜瑕佹眰锛堜粠婧愮爜鏋勫缓锛?- Git銆丆Make銆乂isual Studio 2022 Build Tools锛堝惈 VC 宸ュ叿閾撅級銆?- Miniconda锛堢敤浜庤幏鍙?Qt6锛夛細寤鸿鍒涘缓 qt6env 骞跺畨瑁?qt6-main銆乶inja銆?- Windows PowerShell + winget锛堝彲閫夛級锛氱敤浜庡畨瑁呯己澶变緷璧栥€?

## 鑾峰彇涓庤繍琛?鏈」鐩噰鐢?**build-msvc.zip 鍒嗗嵎**鍙戝竷锛氫笅杞藉悗瑙ｅ帇鍗冲彲杩愯锛屾棤闇€棰濆瀹夎涓庤剼鏈€?
1) 涓嬭浇
- 涓嬭浇 Release 涓殑 `build-msvc.zip.001` ~ `build-msvc.zip.012`锛堝繀椤诲叏閮ㄤ笅杞藉埌鍚屼竴鐩綍锛?
2) 瑙ｅ帇
- 瀹夎 7-Zip锛堟帹鑽愶級鍚庯紝鍦ㄨ鐩綍鎵ц锛?
```cmd
7z x build-msvc.zip.001
```

鎴栧湪璧勬簮绠＄悊鍣ㄩ噷鍙抽敭 `build-msvc.zip.001` 鈫?7-Zip 鈫?瑙ｅ帇銆?
3) 杩愯
- 瑙ｅ帇瀹屾垚鍚庯紝鐩存帴杩愯 `XiaoMo.exe`


## 璧勬簮涓庣洰褰曠害瀹?搴旂敤杩愯鏃舵煡鎵捐祫婧愮殑鏍圭洰褰曚负绋嬪簭鐩綍涓嬬殑 `res`锛?```
XiaoMo.exe
res/
  bin/            # 鍖呭惈 llama-cli.exe/llama.exe 绛夎繍琛屽櫒锛堢敤浜庢湰鍦?LLM锛?  llm/            # 鏈湴 LLM 妯″瀷锛?.gguf锛夛紝鍙垎瑙勬ā瀛愮洰褰?1.5B/7B
  models/         # Live2D 妯″瀷闆嗗悎锛堟瘡涓瓙鐩綍涓€涓ā鍨嬶級
  voice_deps/     # 绂荤嚎璇煶妯″瀷涓庝緷璧栵紙sherpa-onnx 绛夛級
  i18n/, icons/   # 璇█涓庡浘鏍囪祫婧?```

Live2D 妯″瀷锛坮es/models锛夛細
- 姣忎釜妯″瀷鏀惧湪鐙珛鏂囦欢澶癸紝濡?`res/models/<妯″瀷鍚?/`銆?- 妯″瀷鏂囦欢浼樺厛鍖归厤 `*.model3.json`锛岃嫢鏃犲垯灏濊瘯 `*.model.json`锛屽惁鍒欏洖閫€ `model3.json/model.json/index.json`銆?- 榛樿妯″瀷鏍圭洰褰曪紙璁剧疆涓彲鏀癸級锛歚鏂囨。\XiaoMo\Models`銆傞娆¤繍琛屼細鑷姩浣跨敤鍏朵腑鐨勭涓€涓ā鍨嬨€?
鏈湴 LLM 妯″瀷锛坮es/llm锛夛細
- 灏?gguf 鏂囦欢鏀惧叆 `res/llm/1.5B/` 鎴?`res/llm/7B/`锛堝彲涓嶅垎鏂囦欢澶癸紝绋嬪簭浼氳嚜鍔ㄥ尮閰嶏級銆?- 椋庢牸鍖归厤瑙勫垯锛堟枃浠跺悕鍖呭惈浠ヤ笅鍏抽敭瀛椾箣涓€灏嗕紭鍏堥€夋嫨锛夛細
  - Anime锛氬寘鍚?`anime` 鎴?`anime.q`
  - Original锛氬寘鍚?`original` 鎴?`llama`
  - Universal锛氬寘鍚?`universal`
- 妯″瀷瑙勬ā鍦ㄢ€淎I 璁剧疆鈥濋€夋嫨锛?.5B 鏇村揩銆?B 鏇村己锛夛紱鏈懡涓鏍煎叧閿瓧鏃朵細浣跨敤鐩綍涓嬬涓€椤广€?- 鏀寔鐜鍙橀噺瑕嗙洊锛?  - `LLAMA_RUNNER` 鎸囧畾 llama 杩愯鍣ㄨ矾寰?  - `LLM_MODEL` 鎸囧畾 gguf 妯″瀷鏂囦欢璺緞

绂荤嚎 TTS锛坮es/voice_deps锛夛細
- 璇煶妯″瀷鐩綍锛歚res/voice_deps/models/<妯″瀷鍚?/`锛岀▼搴忎細鑷姩鍒楀嚭鍙敤妯″瀷銆?- 澶氳璇濅汉锛坰id锛夛細鑻ユā鍨嬪寘鍚璇濅汉鏄犲皠锛坰peaker_id_map 鎴?speakers.txt锛夛紝鍙湪鈥淎I 璁剧疆鈥濋噷璋冭妭 sid銆?- sherpa-onnx 鍙墽琛岀洰褰曡嚜鍔ㄦ娴嬩簬 `res/voice_deps/sherpa-onnx-*/bin`銆?

## 蹇嵎閿笌鎵樼洏
- Ctrl+T锛氭樉绀?闅愯棌鑱婂ぉ绐楀彛
- Ctrl+H锛氭樉绀?闅愯棌妗屽疇绐楀彛
- Ctrl+S锛氭墦寮€璁剧疆绐楀彛
- 鎵樼洏鍥炬爣锛氬彸閿彍鍗曞彲蹇€熸墦寮€鑱婂ぉ銆佽缃€侀€€鍑猴紱鍒囨崲鏄剧ず/闅愯棌鐘舵€?

## 浣跨敤鎸囧崡锛堣缃獥鍙ｏ級
璁剧疆绐楀彛鍒嗏€滃熀纭€璁剧疆 / 妯″瀷璁剧疆 / AI 璁剧疆 / 楂樼骇璁剧疆鈥濆洓涓〉绛撅細

1) 鍩虹璁剧疆
- 涓婚涓庤瑷€锛氬垏鎹㈠簲鐢ㄤ富棰樹笌鏄剧ず璇█銆?- 绐楀彛琛屼负锛氬叏灞€缃《銆侀€忔槑鑳屾櫙銆侀紶鏍囩┛閫忥紙绌块€忓紑鍚悗闇€ Alt+Tab 鍒囧洖鍐嶅叧闂級銆?- 杈撳嚭姘旀场锛氶€夋嫨瑙掕壊鏃佽竟瀵硅瘽姘旀场鏍峰紡锛堝 鐖卞績/婕敾锛夈€?
2) 妯″瀷璁剧疆锛圠ive2D锛?- 妯″瀷閫夋嫨锛氫粠榛樿妯″瀷鏍圭洰褰曚腑閫夋嫨涓嶅悓妯″瀷锛涘彲涓€閿墦寮€褰撳墠妯″瀷鎵€鍦ㄧ洰褰曘€?- 鍘婚櫎姘村嵃锛氫负褰撳墠妯″瀷閫夋嫨涓€涓?`*.exp3.json` 琛ㄨ揪寮忔枃浠朵綔涓衡€滄按鍗拌〃杈惧紡鈥濓紝浠ヤ究瑕嗙洊骞跺幓闄ゆ按鍗版晥鏋滐紙鍙殢鏃跺彇娑堬級銆?- 鍔ㄤ綔涓庢晥鏋滐細寮€鍏斥€滆嚜鍔ㄥ懠鍚?鑷姩鐪ㄧ溂/瑙嗙嚎璺熻釜/鐗╃悊妯℃嫙鈥濄€?
3) AI 璁剧疆锛堝璇濅笌璇煶锛?- 瑙掕壊鍚嶇О锛氫細鏇挎崲 System Prompt 涓殑 `$name$` 鍙橀噺銆?- 涓婁笅鏂囨潯鏁帮細鍙備笌鏈湴 LLM 瀵硅瘽鐨勫巻鍙叉秷鎭潯鏁帮紙寤鸿 1.5B鈮?2锛?B鈮?4锛夈€?- 鏈€澶х敓鎴愰暱搴︼細鍗曟鐢熸垚鐨勬渶澶?token 鏁帮紙寤鸿 1.5B鈮?92锛?B鈮?84锛夈€?- 瀵硅瘽浜鸿锛圫ystem Prompt锛夛細鑷畾涔夎鑹茶姘斾笌杈圭晫锛屾敮鎸?`$name$` 鍙橀噺銆?- LLM 瑙勬ā涓庨鏍硷細閫夋嫨 1.5B/7B 浠ュ強 Original/Universal/Anime銆?- 绂荤嚎 TTS锛氬惎鐢ㄥ悗鑷姩鏈楄 AI 鏈€缁堝洖澶嶏紱鍙€夋嫨 TTS 妯″瀷銆佽瀹氳璇濅汉 sid 涓庨煶閲忋€?
4) 楂樼骇璁剧疆
- 鎶楅敮榻?MSAA锛氳皟鑺傛覆鏌撻噰鏍凤紙2x/4x/8x锛夈€?- 妯″瀷鏄剧ず灞忓箷锛氬灞忕幆澧冨彲鎸囧畾鏄剧ず灞忋€?- 瀹氭椂浠诲姟锛氭敮鎸佹瘡澶╁浐瀹氭椂闂存垨鎸夐棿闅旇Е鍙戠殑鏈湴鎻愰啋銆?

## 浠庢簮鐮佹瀯寤猴紙鎵嬪姩姝ラ姒傝锛?鍙傝€冧互涓嬫墜鍔ㄦ祦绋嬶紙绠€鐗堬級锛?
1) 鑾峰彇渚濊禆
- 瀹夎 Git銆丆Make銆乂S 2022 Build Tools锛堝惈 VC 宸ュ叿閾撅級銆?- 瀹夎 Miniconda 骞跺垱寤虹幆澧冿細`conda create -y -n qt6env -c conda-forge qt6-main ninja`

2) 鎷夊彇涓庨厤缃?- 鍏嬮殕浠撳簱锛歚git clone https://github.com/linhanmo/desktop-pet-with-llms.git`
- 鍑嗗 Qt6锛氭壘鍒?`Qt6_DIR` 涓?`CMAKE_PREFIX_PATH`锛堥€氬父鍦?`envs/qt6env/Library/lib/cmake/Qt6` 涓?`envs/qt6env/Library`锛夈€?
3) CMake 鏋勫缓
- 鐢熸垚锛歚cmake -S Pet -B Pet/build -G "Visual Studio 17 2022" -A x64 -DQt6_DIR=... -DCMAKE_PREFIX_PATH=...`
- 缂栬瘧锛歚cmake --build Pet/build --config Release -j 8`
- 閮ㄧ讲锛氫娇鐢?`windeployqt6.exe` 鏀堕泦杩愯鎵€闇€ DLL 鍒板彲鎵ц鐩綍銆?
4) 鏀剧疆璧勬簮
- 灏?`cubism` SDK锛堣В鍘嬪悗涓?`sdk/cubism`锛変笌 `models/`銆乣voice_deps/`銆乣llm/` 绛夎В鍘?鏀剧疆鍒?`Pet/res` 涓嬶紙璇﹁鈥滆祫婧愪笌鐩綍绾﹀畾鈥濓級銆?

## 甯歌闂
- 杩愯鍚庢棤瀵硅瘽/鍥炲寰堢煭
  - 纭 `res/bin` 涓瓨鍦?`llama-cli.exe/llama.exe`锛屽苟鍦ㄢ€淎I 璁剧疆鈥濅腑閫夋嫨鍚堥€傝妯′笌椋庢牸銆?  - 鏀惧叆鍖归厤鐨?gguf 妯″瀷鑷?`res/llm/1.5B` 鎴?`res/llm/7B`锛涙垨璁剧疆鐜鍙橀噺 `LLM_MODEL` 鎸囧畾璺緞銆?- 鏃犳硶鍙戝０/鏃犲彲閫?TTS 妯″瀷
  - 纭 `res/voice_deps/models` 涓嬪瓨鍦ㄦā鍨嬪瓙鐩綍锛屼笖鍖呭惈妯″瀷/閰嶇疆鏂囦欢锛涘惎鐢ㄢ€滅绾?TTS鈥濄€?  - 澶氳璇濅汉妯″瀷闇€姝ｇ‘鐨勮璇濅汉鏄犲皠鏂囦欢锛堝 speakers.txt 鎴?JSON 鏄犲皠锛夈€?- 鍚堝苟/瑙ｅ帇澶辫触
  - 纭 `build-msvc.zip.001`~`.012` 鍏ㄩ儴涓嬭浇瀹屾垚骞朵綅浜庡悓涓€鐩綍锛涚敤 7-Zip 浠?`.001` 寮€濮嬭В鍘嬨€?

## 绀轰緥鍛戒护锛堥€熸煡锛?
- 瑙ｅ帇 build-msvc 鍒嗗嵎

```cmd
7z x build-msvc.zip.001
```

- 鍒嗗嵎鍚堝苟锛堝彲閫夎祫婧愭墜鍔級

```cmd
:: 鍚堝苟绂荤嚎璇煶渚濊禆
copy /b voice_deps.zip.001+voice_deps.zip.002 voice_deps.zip

:: 鍚堝苟 LLM 璧勬簮鍖?copy /b llm.zip.001+llm.zip.002+llm.zip.003+llm.zip.004+llm.zip.005+llm.zip.006+llm.zip.007+llm.zip.008+llm.zip.009 llm.zip
```

- 灏嗚祫婧愯В鍘嬪埌姝ｇ‘浣嶇疆锛堟墜鍔級

```powershell
# 鍋囪褰撳墠鍦ㄤ粨搴?Pet 鐩綍
Expand-Archive -LiteralPath .\assets\cubism.zip -DestinationPath .\sdk -Force
Rename-Item -Path .\sdk\cubism* -NewName cubism -ErrorAction SilentlyContinue
Expand-Archive -LiteralPath .\assets\models.zip -DestinationPath .\res -Force
7z x .\assets\voice_deps.zip.001 -o.\res -y
New-Item -ItemType Directory -Force -Path .\res\llm | Out-Null
7z x .\assets\llm.zip.001 -o.\res\llm -y
```

- 鎸囧畾 LLM 杩愯鍣ㄤ笌妯″瀷锛堢幆澧冨彉閲忚鐩栵級

```powershell
$env:LLAMA_RUNNER = "E:\XiaoMo\res\bin\llama-cli.exe"
$env:LLM_MODEL    = "E:\XiaoMo\res\llm\1.5B\your-model.gguf"
# 杩愯 XiaoMo.exe 鍚庡皢浣跨敤浠ヤ笂璁剧疆锛涘叧闂?PowerShell 璇ヨ缃け鏁?```

- llama-cli 蹇€熻嚜妫€

```powershell
.\res\bin\llama-cli.exe -m .\res\llm\1.5B\your-model.gguf -p "浣犲ソ" --simple-io -n 64
```

- 鎵嬪姩瀹夎鏋勫缓渚濊禆锛堝彲閫夛級

```powershell
winget install --id Git.Git -e --accept-package-agreements --accept-source-agreements
winget install --id Kitware.CMake -e --accept-package-agreements --accept-source-agreements
winget install --id 7zip.7zip -e --accept-package-agreements --accept-source-agreements
winget install --id Microsoft.VisualStudio.2022.BuildTools -e --override "--quiet --wait --norestart --add Microsoft.VisualStudio.Component.VC.Tools.x86.x64"
```

- 鍑嗗 Qt6 鐨?Conda 鐜锛堝彲涓庤剼鏈竴鑷达級

```powershell
conda create -y -n qt6env -c conda-forge qt6-main ninja
```


## 閰嶇疆涓庢棩蹇楄矾寰?- 閰嶇疆鐩綍锛圵indows锛夛細`%APPDATA%\IAIAYN\XiaoMo\Configs\config.json`
- 鏈湴鏁版嵁鐩綍锛圵indows锛夛細`%LOCALAPPDATA%\IAIAYN\XiaoMo`
- 鍚姩鏃ュ織锛歚%LOCALAPPDATA%\... \logs\startup.log`


## 璁稿彲璇佷笌鑷磋阿
- 鏈」鐩娇鐢ㄧ殑绗笁鏂圭粍浠朵笌妯″瀷鐗堟潈褰掑叾鍚勮嚜鎵€鏈夎€呮墍鏈夈€侺ive2D 妯″瀷鍜岃闊虫ā鍨嬭閬靛惊鍏跺搴旀巿鏉冨崗璁€?

## 寮€鍙戣€呬俊鎭笌娴嬭瘯浜哄憳鑷磋阿
- 寮€鍙戣€咃細Mo 
- 娴嬭瘯浜哄憳锛歡uos7898-alt , xpresent-10

涓囧垎鎰熻阿鎵€鏈夋祴璇曚汉鍛樼殑鍙嶉涓庡缓璁紝浠栦滑甯姪鎴戝叏闈紭鍖栦簡椤圭洰鐨勬牳蹇冩€ц兘涓庤繍琛岀ǔ瀹氭€э紝鎵撶（骞舵彁鍗囦簡浜у搧鍏ㄩ摼璺殑浣跨敤浣撻獙锛屾洿绮惧噯鎺掓煡瀹氫綅浜嗗澶勬綔鍦ㄧ殑绋嬪簭缂洪櫡涓庨闄╅殣鎮ｏ紝涓洪」鐩殑椤哄埄钀藉湴涓庨暱鏈熷钩绋宠繍琛岀瓚鐗簡鍧氬疄鏍瑰熀銆?
鐧惧害缃戠洏閾炬帴: https://pan.baidu.com/s/13QY8_rEd1pWLyFOL9JsFVQ?pwd=8888 鎻愬彇鐮? 8888
