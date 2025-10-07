module user_ip (
  inout              LED_TEST1,
  inout              LED_TEST2,
  output              DAC_SCLK,
  output              DAC_CSN,
  output              DAC_MOSI,
  inout					LED_USER,
  input              iocvt_chn_out_data,
  input              iocvt_chn_out_en,
  input              sys_clock,
  input              bus_clock,
  input              resetn,
  input              stop,
  input       [1:0]  mem_ahb_htrans,
  input              mem_ahb_hready,
  input              mem_ahb_hwrite,
  input       [31:0] mem_ahb_haddr,
  input       [2:0]  mem_ahb_hsize,
  input       [2:0]  mem_ahb_hburst,
  input       [31:0] mem_ahb_hwdata,
  output tri1        mem_ahb_hreadyout,
  output tri0        mem_ahb_hresp,
  output tri0 [31:0] mem_ahb_hrdata,
  output tri0        slave_ahb_hsel,
  output tri1        slave_ahb_hready,
  input              slave_ahb_hreadyout,
  output tri0 [1:0]  slave_ahb_htrans,
  output tri0 [2:0]  slave_ahb_hsize,
  output tri0 [2:0]  slave_ahb_hburst,
  output tri0        slave_ahb_hwrite,
  output tri0 [31:0] slave_ahb_haddr,
  output tri0 [31:0] slave_ahb_hwdata,
  input              slave_ahb_hresp,
  input       [31:0] slave_ahb_hrdata,
  output tri0 [3:0]  ext_dma_DMACBREQ,
  output tri0 [3:0]  ext_dma_DMACLBREQ,
  output tri0 [3:0]  ext_dma_DMACSREQ,
  output tri0 [3:0]  ext_dma_DMACLSREQ,
  input       [3:0]  ext_dma_DMACCLR,
  input       [3:0]  ext_dma_DMACTC,
  output tri0 [3:0]  local_int
);
//assign mem_ahb_hreadyout = 1'b1;
//assign slave_ahb_hready  = 1'b1;

//assign LED_TEST1 = iocvt_chn_out_data;

//assign LED_TEST2 = !iocvt_chn_out_data;

assign LED_USER = iocvt_chn_out_data;


parameter ADDR_BITS = 16;    //真正使用到的地址位宽
parameter DATA_BITS = 32;    //真正使用到的数据位宽

//定义出：（ahb转apb后）apb的信号线
wire                 apb_psel;
wire                 apb_penable;
wire                 apb_pwrite;
wire [ADDR_BITS-1:0] apb_paddr;
wire [DATA_BITS-1:0] apb_pwdata;
wire [3:0]           apb_pstrb;
wire [2:0]           apb_pprot;
wire                 apb_pready  = 1'b1;
wire                 apb_pslverr = 1'b0;
wire [DATA_BITS-1:0] apb_prdata;

//apb的clock使用bus_clock。也可以使用其他clock。
//如果是慢速设备，请自行决定apb_clock的时钟频率。
assign apb_clock = bus_clock;

//关联ahb2apb模块。
//这里的#(ADDR_BITS, DATA_BITS) 会改写ahb2apb对应的宏的值。
ahb2apb #(ADDR_BITS, DATA_BITS) ahb2apb_inst(
  .reset        (!resetn                     ),
  .ahb_clock    (sys_clock                   ),
  .ahb_hmastlock(1'b0                        ),
  .ahb_htrans   (mem_ahb_htrans              ),
  .ahb_hsel     (1'b1                        ),
  .ahb_hready   (mem_ahb_hready              ),
  .ahb_hwrite   (mem_ahb_hwrite              ),
  .ahb_haddr    (mem_ahb_haddr[ADDR_BITS-1:0]),
  .ahb_hsize    (mem_ahb_hsize               ),
  .ahb_hburst   (mem_ahb_hburst              ),
  .ahb_hprot    (4'b0011                     ),
  .ahb_hwdata   (mem_ahb_hwdata              ),
  .ahb_hrdata   (mem_ahb_hrdata              ),
  .ahb_hreadyout(mem_ahb_hreadyout           ),
  .ahb_hresp    (mem_ahb_hresp               ),
  .apb_clock    (apb_clock                   ),
  .apb_psel     (apb_psel                    ),
  .apb_penable  (apb_penable                 ),
  .apb_pwrite   (apb_pwrite                  ),
  .apb_paddr    (apb_paddr                   ),
  .apb_pwdata   (apb_pwdata                  ),
  .apb_pstrb    (apb_pstrb                   ),
  .apb_pprot    (apb_pprot                   ),
  .apb_pready   (apb_pready                  ),
  .apb_pslverr  (apb_pslverr                 ),
  .apb_prdata   (apb_prdata                  )
);
//以上两部分，一组是mcu到ahb的信号(mem_ahb_xxxx)，一组是ahb转apb之后的信号（apb_xxxx)
//这里以下，可以直接使用apb_xxxx信号，遵循APB协议即可。


parameter DAC8830_ADDR = 'h6000;		//对应mcu里的0x60006000, cpld中仅用相对偏移
//如果cpld下要挂多个外设，可以从这里的分段来区分不同设备，使用“片选”的概念。

parameter ADDR_DAC8830_DATA = 'h00;	//写的寄存器
parameter ADDR_DAC8830_STAT = 'h04;	//读的寄存器，最低位为busy位，倒数第二位为tc位。


//测试用的led。cpld中不能debug，只能靠led闪灯来帮助判定运行情况。
reg TestLedCtrl = 1;
assign LED_TEST1   = TestLedCtrl;

reg dac_start;		//是否启动
reg dac_reset;		//是否reset
reg[15:0] dac_write;	//mcu写的数据线（即，要通过串口发出的数据），32位中取末8位
wire tx_busy;		   //是否正在发送中
wire tx_complete;    //是否发送完成
//wire uart_tx_line;	//发送uart数据，要操作的tx信号线，就是VE里定义的 PIN_17
reg [31:0] prdata;	//mcu读的数据线
//上边的tx_busy和tx_complete是返回给mcu的状态。
//这些标记按自己需求定义，跟mcu端能实现信息交互即可。


//assign UART_TX = uart_tx_line;
wire apb_data_phase = apb_psel && apb_penable;	//等于apb_penable。在这里apb_psel恒为1
assign apb_prdata = prdata;

// 将APB过来的数据通过SPI发送出去
always @ (posedge apb_clock or negedge resetn) begin
  if (!resetn) //开始
    begin
      dac_reset <= 0;
      dac_start <= 0;
		TestLedCtrl <= 1; // LED默认关闭
	end 
  //mcu的写操作。
  //mcu端用C语言：*((int *)0x60006000) = value;
  else if (apb_data_phase && apb_pwrite && apb_paddr == DAC8830_ADDR + ADDR_DAC8830_DATA) //收到mcu写过来的数据
    begin
      TestLedCtrl <= 0;					//led测试，低为led亮
		dac_reset <= 1;
      dac_write <= apb_pwdata[15:0];	//只传低16位。dac_write线路会连到DAC8830模块，靠DAC8830模块真正根据时序往外发。
      dac_start <= 1;
      
    end
  //mcu的读操作响应
  //mcu端用C语言：int value = *((int *)0x60006004);
  else if (apb_data_phase && !apb_pwrite && apb_paddr == DAC8830_ADDR + ADDR_DAC8830_STAT) //收到mcu读取state寄存器，将UART的状态通过APB返回回去
    begin
      prdata <= {30'b0, tx_complete, tx_busy};  //最低位为busy位，倒数第二位为tc位，高30位无效
		//if (tx_complete == 1) 
		TestLedCtrl <= 1; //led测试
    end
  else 
    begin  
      dac_start <= 0;
		dac_reset <= 1;
    end
end


// dac8830_driver模块
dac8830_driver dac8830_driver_ins(
    .clk(apb_clock),
    .rst_n(dac_reset),
    .start(dac_start),
    .data(dac_write),
    .sclk(DAC_SCLK),
    .cs_n(DAC_CSN),
    .mosi(DAC_MOSI),
    .busy(tx_busy),
    .done(tx_complete)
);









endmodule
