# 蓝牙连接

本示例在广播的基础上，添加了可连接的功能。

## 代码解析

程序主要通过```app.c```文件中的```user_init_normal(void)```函数中实现蓝牙初始化，主要代码如下：

	void user_init_normal(void)
	{
		random_generator_init();  //初始化随机数生成器

		u8  mac_public[6];
		u8  mac_random_static[6];
		blc_initMacAddress(CFG_ADR_MAC, mac_public, mac_random_static); //初始化MAC地址

		////// Controller Initialization  //////////
		blc_ll_initBasicMCU();   //初始化MCU
		blc_ll_initStandby_module(mac_public);		//初始化蓝牙待机功能模块
		blc_ll_initAdvertising_module(mac_public);  //初始化蓝牙广播功能模块
		blc_ll_initConnection_module();				//初始化蓝牙连接功能模块
		blc_ll_initSlaveRole_module();				//初始化蓝牙从机功能模块

		blc_smp_setSecurityLevel(No_Security); //设置连接安全级别

		u8 tbl_advData[] = {0x05, 0x09, 'A', 'B', 'C', 'D'}; //要广播的数据

		bls_ll_setAdvData( (u8 *)tbl_advData, sizeof(tbl_advData) ); //设置广播数据

		bls_ll_setScanRspData(NULL,0); //设置扫描响应数据空

		u8 status = bls_ll_setAdvParam( ADV_INTERVAL_50MS , //广播时间间隔最小值
										ADV_INTERVAL_50MS , //广播时间间隔最大值
										ADV_TYPE_CONNECTABLE_UNDIRECTED, //广播类型，可连接非定向
										OWN_ADDRESS_PUBLIC, //自身地址类型
										0,  //定向地址类型
										NULL, //定向地址
										BLT_ENABLE_ADV_ALL, //在全部广播信道(37,38,39)都广播数据
										ADV_FP_NONE); //过滤规则

		if(status != BLE_SUCCESS)//如果设置广播参数失败
		{
			write_reg8(0x40000, 0x11);  //debug
			while(1);
		}

		rf_set_power_level_index (MY_RF_POWER_INDEX); //设置发射功率

		bls_ll_setAdvEnable(1);  //开启广播

		bls_app_registerEventCallback (BLT_EV_FLAG_CONNECT, &connect_callback);  //注册蓝牙连接建立回调函数
		bls_app_registerEventCallback (BLT_EV_FLAG_TERMINATE, &disconnect_callback); //注册蓝牙连接断开回调函数

		bls_app_registerEventCallback (BLT_EV_FLAG_CONN_PARA_REQ, &conn_para_req_callback);//注册蓝牙连接参数请求回调函数
		bls_app_registerEventCallback (BLT_EV_FLAG_CONN_PARA_UPDATE, &conn_para_up_callback);//注册蓝牙连接参数更新回调函数

		app_uart_init(); //初始化串口，用于调试打印输出

		irq_enable();
	}

打开手机上的蓝牙助手APP，搜索名称为ABCD的设备，点击连接，串口将会输出 ```+BLE_CONNECTED```,断开连接后，串口将会输出```+BLE_DISCONNECTED```。

## 实现原理

本示例在广播示例的基础上添加了 ```blc_ll_initConnection_module();```和 ```blc_ll_initSlaveRole_module();```这两个功能模块，广播类型设置为了```可连接非定向```。然后注册了四个蓝牙时间回调函数，用于指示连接状态。

## 其他改动

### fifo

SDK协议栈收发数据需要定义如下buff：
	attribute_data_retention_  u8 		 	blt_rxfifo_b[RX_FIFO_SIZE * RX_FIFO_NUM] = {0};
	attribute_data_retention_	my_fifo_t	blt_rxfifo = {
													RX_FIFO_SIZE,
													RX_FIFO_NUM,
													0,
													0,
													blt_rxfifo_b,};

	_attribute_data_retention_  u8 		 	blt_txfifo_b[TX_FIFO_SIZE * TX_FIFO_NUM] = {0};
	_attribute_data_retention_	my_fifo_t	blt_txfifo = {
													TX_FIFO_SIZE,
													TX_FIFO_NUM,
													0,
													0,
													blt_txfifo_b,};


### 设置连接安全级别
通过```blc_smp_setSecurityLevel()``` 函数可设置连接的安全级别，本示例中使用的安全级别为No_Security，即不需要任何验证即可连接。

### 扫描响应
由于我们将广播类型设为了```可连接非定向```，所以就默认开启了扫面响应，需设置扫描响应数据，如果不是这将会采用SDK里面默认的扫描响应数据。代码中通过```bls_ll_setScanRspData(NULL,0);```，将扫面响应数据设为空。