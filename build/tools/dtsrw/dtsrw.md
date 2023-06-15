dts在我们linux应用中以dtb形式存在flash的一段固定数据块中，目前位置是0x8000（524288）， 长度是0x10000（65536)。可以通过查看hcprog.ini得到相应信息。

uboot，kernel，avp都使用到dts，但目前uboot的dts是编译进uboot了的；linux和avp则可以使用flash中的dts。将来最好同一使用flash中的同一份dts，这样就可能方便使用工具修改flash中的dts，而无需重新编译代码。



## 描述

dtsrw.sh脚本可以从固件中读取dtb，转换成dts，用户可以修改对应dts的一些配置；然后脚本还可以再把修改的dts写入固件中，这样一些kernel，avp使用的dts信息就可以直接修改生效了。但目前需要注意uboot还是使用原来内嵌的dts配置。



dtsrw.sh使用了两个工具： binrw和dtc

binrw是自己写的工具，可以把输入文件指定位置，长度的数据写入输出文件指定位置；

dtc是开源工具，开源把dts和dtb互相转换。因此，linux环境先安装dtc工具

## 工作原理：

从固件读取到dts文件：

* binrw把输入固件的dtb读取出来，保存成tmp.dtb文件；

* dtc把tmp.dtb转换成dts文件

修改dts文件，重新写入固件：

* 手动修改dts对应参数
* dtc把dts转换成tmp.dtb
* binrw把tmp.dtb写入固件



## 使用方法

* 检查脚本dtsrw.sh，看是否需要修改dtb在固件中的位置和长度

  ```
  #In general, the position of dtb bin data in the firmware is 0x80000(524288), 
  #and the length is 0x10000(65536)
  #can check these parameters in hcprog.ini
  DTB_OFFSET=524288
  DTB_LENGTH=65536
  
  ```

  

* 读取固件的dtb，转换成my.dts

./dtsrw.sh -r sfburn.bin my.dts

* 把my.dts写入固件

./dtsrw.sh -w my.dts sfburn.bin



## 修改

可以直接修改binrw.c源文件，重新编译工具

gcc binrw.c -o binrw