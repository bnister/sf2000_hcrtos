# Display Setting Web

# 1. 配置说明

## 1.1 语言配置

* 配置文件：`config_lang.js`
* 新增语言：复制一份其他语言，然后以i18标准语言编码为Key(简易)，然后翻译每一项即刻
* 删除语言：在配置文件中删除语言对应的全部内容即可

## 1.2 功能配置

* 配置文件：`config_menu.js`
* 功能开关：在配置文件的中，把对应功能的值设置为`true/false`即可

## 1.3 设置项配置

* 配置文件: `config_setting.js`
* 配置项目说明：

| 属性名		| 示例值 		| 必须	| 说明																				|
|---------------|---------------|-------|-----------------------------------------------------------------------------------|
| use			| true			| 是	| 是否使用此配置项																	|
| name       	| device_name	| 是	| 选项名，需对应多语言项															|
| key     		| device_name	| 是	| 选项键，需对应机器设置项(setting_get的值)											|
| component 	| input_text	| 是	| 输入控件(input_text:文本输入，input_password:密码输入，input_select:下拉选择)		|
| default		| true			| 否	| 默认值																			|
| confirm		| true 			| 否	| 设置时是否进行操作提示 															|
| confirm_tips	| 1				| 否	| 设置时操作提示的提示信息 															|
| items     	| {"1":"4:3"}	| 否	| 下拉选择项，配合input_select使用													|
| max_length 	| 8				| 否	| 最大输入长度，配合input_text使用													|
| min_length 	| 8				| 否	| 最小输入长度，配合input_text使用													|
| max_value 	| 100			| 否	| 输入最大值，配合input_text使用													|
| min_value 	| 0				| 否	| 输入最小值，配合input_text使用													|
| input_limit	| base			| 否	| 输入值限制，配合input_text使用(base:过滤非法特殊字符，number:限制输入数字) 		|
| enabled_key	| resolution	| 否	| 判断选项是否可用，对应设备配置信息中的字段										|
| enabled_val	| 1				| 否	| 判断选项是否可用，当enabled_key的值等于此配置时，选项可用							|
| use_key		| 4k_use		| 否	| 判断选项是否启用，对应设备配置信息中的字段										|
| use_val		| 1				| 否	| 判断选项是否启用，当use_key的值等于此配置时，选项启用								|

* 配置枚举选项说明：

| 属性名		| 示例值 		| 必须	| 说明															|
|---------------|---------------|-------|---------------------------------------------------------------|
| key			| 1				| 是	| 枚举选项实际值												|
| name       	| 480P			| 是	| 枚举选项显示值(会配合语言配置进行翻译)						|
| use_key		| resolution_4k	| 否	| 判断枚举选项是否启用，对应设备配置信息中的字段  				|
| use_val		| enabled		| 否	| 判断枚举选项是否启用，当use_key的值等于此配置时，枚举项启用 	|

## 1.4 设置产品名称与图标

* 修改产品图标：直接替换导出文件包中`favicon.ico`即
* 修改产品名称：在语言配置文件(`config_lang.json`)中修改各语言中的`project_name`项即可
* 修改项目标题：在语言配置文件(`config_lang.json`)中修改各语言中的`title`项即可
> 产品名称与图标主要在浏览器选项卡中显示，项目标题主要显示在设置页面的首页标题中。

# 2. 接口说明

## 2.1 关于获取机器当前设置

* 获取机器配置接口为: `/setting_get` (GET)
* 设置项对照表：

| 项目					| 描述								|
| --------------------- | --------------------------------- |
| lang					| 语言								|
| product				| 产品								|
| version				| 版本								|
| upgrade_api			| 升级服务接口地址(JSON)   			|
| chip					| Chip ID							|
| vendor				| 厂商								|
| mac					| MAC地址							|
| networking			| 是否连网(废弃)  					|
| device_name			| 设备名称							|
| device_password		| 设备密码							|
| wifi_mode 			| WiFi模式							|
| wifi_mode 			| WiFi模式							|
| wifi_channel_enabled	| WiFi信道							|
| resolution			| 分辨率  							|
| android_mode			| 安卓模式							|
| mirror_model			| 镜像模式							|
| aircast_mode			| iOS同屏模式						|
| tv_ratio				| 比例								|
| bright				| 亮度								|
| contrast				| 对比度  							|
| saturation			| 饱和度  							|
| hue					| 色调 								|
| sharpness 			| 锐度 								|

* 需要返回的结果如：
```
{
	"lang": "2",
	"product": "A210",
	"version": "1.0.0-150000",
	"upgrade_server": "http://127.0.0.1/A210.jsonp",
	"chip": "5799c753092289da",
	"vendor": "hichip",
	"mac": "AP:IO:BF:91:D2:0C",
	"networking": true,
	"device_name": "APIDEV",
	"device_password": "88888888",
	"wifi_mode": "1",
	"wifi_mode_enabled": true,
	"resolution": "1",
	"android_model": "1",
	"mirror_model": "3",
	"aircast_mode": "2",
	"tv_ratio": "1",
	"bright": "50",
	"contrast": "50",
	"saturation": "50",
	"hue": "50",
	"sharpness": "10"
};
```
* 语言的枚举项为：
```
{
    "1" : "English",
    "2" : "简体中文",
    "3" : "繁體中文",
}
```
* 升级服务接口格式为：
```
jsonp_callback({
  "product": "HC15A210",
  "version": "2303061322",
  "url": "http://127.0.0.1/upgrade/HCFOTA_HC15A210_2303061322.bin"
})
```
* 设置的枚举项参考配置文件`config_setting.js`中对应项的`items`

## 2.2 关于提交设置变更

* 设置变更提交的接口为: `/setting_submit?key={key}&valul={value}` (GET)
    * `key`: 变更项
    * `value`: 变更值
* 注意事项1：多语言中的当前语言码时保存在浏览器的`localStorage`中,并不会提交更变
* 注意事项2: `key`值做了一些变更，命名从驼峰法变更为下划线
* 接口返回数据格式示例：
```
{
	"changes": {
		"mirror_rotation": "1"
	}
}
```
* 返回对象属性说明

| 属性名	| 示例值 			| 说明 														|
|-----------|-------------------|-----------------------------------------------------------|
| changes	| {"key":"value"}	| 值修改成功后,其他需要变更的关联值列表(用于通知前端显示)	|


## 2.3 关于获取wifi列表接口

* 获取wifi列表接口为: `/wifi_get` (GET)
* 返回数据例子如下：
    * `index`: 序号
    * `ssid`: 网络名称
    * `bssid`: 网络BSSID
    * `state`: 状态(1:已连接，0:未连接)
    * `intensity`: 信号强度(由强到弱为3-1)
    * `encryption`: 是否加密
    * `saved`: 是否保存
    * `encryption_type`: 加密模式
```
[
    {
        "index": 0,
        "ssid": "api_wifi_1",
        "bssid": "96:66:62:96:72:11",
        "state": 1,
        "intensity": 3,
        "encryption": true,
        "saved": true,
        "encryption_type": "WAP"
    }, {
        "index": 1,
        "ssid": "api_wifi_2",
        "bssid": "96:66:62:96:72:12",
        "state": 0,
        "intensity": 3,
        "encryption": true,
        "saved": true,
        "encryption_type": "WAP"
    }, {
        "index": 2,
        "ssid": "api_wifi_3",
        "bssid": "96:66:62:96:72:13",
        "state": 0,
        "intensity": 2,
        "encryption": false,
        "saved": true,
        "encryption_type": "WAP"
    }
]
```

## 2.4 关于获取wifi连接接口

* WIFI连接接口为: `/wifi_submit?index={index}&ssid={ssid}&bssid={bssid}&password={password}&encryption_type={encryption_type}` (GET)
    * `ssid`: SSID，网络名称
    * `bssid`: BSSID
    * `password`：WIFI密码
    * `encryption_type`：加密类型

## 2.5 关于添加并连接隐藏wifi

* WIFI连接接口为: `/add_hiddenwifi?ssid={ssid}&password={password}&safety={safety}` (GET)
    * `ssid`: SSID，网络名称
    * `password`：WIFI密码
    * `safety`: 为安全性，即加密方式
* 安全性对照表：

| 安全性			| 值		|
| ----------------- | --------- |
| None              | None      |
| WEP               | WEP       |
| WAP/WPA2          | WAP_WPA2  |

## 2.6 其他WIFI相关接口

* WIFI断开连接接口为:`/wifi_unconnect?index={index}&ssid={ssid}&bssid={bssid}` (GET)
* WIFI取消保存接口为:`/wifi_unsaved?index={index}&ssid={ssid}&bssid={bssid}` (GET)

## 2.7 本地升级包升级接口

* 本地包提交的接口为: `/upload_submit` (POST)
* 升级包的文件内容在POST的内容体中

## 2.8 关于其他功能接口

* 升级的调用接口为: `/upgrade_submit?url={url}` (GET)
* 中断更新接口为：`/upgrade_interrupt` (GET)
    * 中断成功返回状态码：`200`
    * 中断失败返回状态码：`500`
* 重启动的调用接口为: `/misc_submit?type=restart` (GET)
* 恢复出厂设置的调用接口为: `/misc_submit?type=reset` (GET)

## 2.9 关于语言包配置

* 配置文件为: `config_lang.js`
* 配置方法: 复制其他语言的配置，配置到相应I18N编码下并进行条目翻译就行了
  * 也可以不按照I18N规范进行命名，但会影响到`lang=0`时识别用户浏览器语言的功能
* 例如（以添加韩语为例）
  * 复制`config_lang.js`中`config_lang`的成员属性`zh_CN`
  * 把复制的`zh_CN`修改为`ko_KR`
  * 然后把逐个文字项翻译成对应的韩文即可
```
var config_lang = {
  "zh_CN":{
    //原本的中文翻译条目
  },
  "ko_KR":{
    "project_name"              : "HCCast",
    "lang_name"                 : "韩语",
    "title"                     : "동스크린 장치 관리",
    "wlan"                      : "WLAN",
    "setting"                   : "설치",
    "language_change"           : "언어 전환",
    "upgrade"                   : "업그레이드",
    //其他韩语翻译条目……
  }
```

## 2.10 获取文件列表(文件管理器)

* 接口: `GET file_list?path={path}`
* 说明：打开文件管理器时，会默认先请求路径为`/`的文件列表
* 接口返回数据格式示例：
```
[
  {
    "filename" : "/dir1",
    "name" : "dir1",
    "type" : "dir",
  },
  {
    "filename" : "/test.jpg",
    "name" : "test.jpg",
    "type" : "image",
  },
  {
    "filename" : "/video.mp4",
    "name" : "video.mp4",
    "type" : "video",
  },
]
```
* 返回对象属性说明

| 属性名		| 示例值 	| 说明  						|
| ------------- | --------- | ----------------------------- |
| filename		| "/dir1"	| 文件全名（包含路径）			|
| name			| "dir1"	| 文件名						|
| type			| dir		| 文件类型，显示根据文件类型 	|

* 文件类型说明

| 属性名		| 示例值 			|
| ------------- | ----------------- |
| dir			| 文件夹 			|
| image 		| 图片				|
| video 		| 视频				|
| image_icon	| 图片(以图标显示) 	|

## 2.11 进行文件删除(文件管理器)

* 接口: `GET file_delete?filename={filename}`
* 说明：进行文件删除请求
* 成功删除：HTTP返回码为200
* 删除失败：HTTP返回码为非200

## 2.12 进行文件上传(文件管理器)

* 接口：`POST file_upload`
* 说明：进行文件上传
* 请求格式说明：上传请求的`Content-type: multipart/form-data`
* 请求参数说明：

| 属性名	| 示例值				|
|-----------|-----------------------|
| path		| 上传文件的目标路径	|
| file		| 上传文件的内容		|

* 请求格式报文示例：

```
-----------------------------7610638442866498471879900459
Content-Disposition: form-data; name="path"

{文件路径值}
-----------------------------7610638442866498471879900459
Content-Disposition: form-data; name="file"; filename="README.md"
Content-Type: application/octet-stream

{文件内容值}
```