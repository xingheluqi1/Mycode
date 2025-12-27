#include <reg51.h>

// --- 宏定义 ---
#define uchar unsigned char // 将 unsigned char 定义为 uchar，方便书写
#define uint unsigned int   // 将 unsigned int 定义为 uint，方便书写

// --- 全局变量定义 ---

// 共阳极数码管段选码表 (0-9)
// 0xC0 代表 gfedcba = 11000000，即a,b,c,d,e,f段亮，g段灭，显示数字 '0'
// 其他以此类推
uchar code seg_code[] = {0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8, 0x80, 0x90};

uchar time_num = 24; // 计时变量，初始值为24秒
bit pause_flag = 0;  // 暂停标志位，0表示运行，1表示暂停
bit alarm_flag = 0;  // 报警标志位，0表示不报警，1表示报警

// --- 函数声明 ---
void delay(uint t);
void display(uchar num);
void timer0_init();
void key_scan();
void alarm_ctrl();

// --- 函数定义 ---

/**
 * @brief  简单的延时函数
 * @param  t 延时参数，t越大，延时越长
 * @note   该延时函数的精确度不高，仅用于按键消抖和数码管动态显示的延时
 */
void delay(uint t)
{
  uint i, j;
  for (i = t; i > 0; i--)
    for (j = 110; j > 0; j--)
      ;
}

/**
 * @brief  数码管显示函数
 * @param  num 要显示的数字 (0-99)
 * @note   通过动态扫描的方式驱动两位数码管显示
 */
void display(uchar num)
{
  uchar shi, ge;
  shi = num / 10; // 计算出十位数字
  ge = num % 10;  // 计算出个位数字

  // 显示十位
  P2 = 0x01;          // P2.0 置为0，选中十位数码管（共阳极管，低电平点亮）
  P0 = seg_code[shi]; // 送出十位的段选码
  delay(1);           // 延时一小段时间，使人眼能看到

  // 显示个位
  P2 = 0x02;         // P2.1 置为0，选中个位数码管
  P0 = seg_code[ge]; // 送出个位的段选码
  delay(1);          // 延时一小段时间
}

/**
 * @brief  定时器0初始化函数
 * @note   设置定时器0为16位定时器模式，定时50ms，并开启相关中断
 */
void timer0_init()
{
  TMOD = 0x01; // 设置定时器0为工作模式1 (16位定时器/计数器)
  TH0 = 0x3C;  // 设置定时器高8位初值 (50ms@12MHz晶振)
  TL0 = 0xB0;  // 设置定时器低8位初值
  ET0 = 1;     // 开启定时器0中断允许
  EA = 1;      // 开启总中断
  TR0 = 0;     // 关闭定时器0，等待启动
}

/**
 * @brief  定时器0中断服务函数
 * @note   每50ms进入一次中断，用于实现精确的1秒计时
 */
uint cnt = 0; // 用于计数中断次数的全局变量
void timer0_isr() interrupt 1
{
  TH0 = 0x3C; // 重新装载高8位初值
  TL0 = 0xB0; // 重新装载低8位初值
  cnt++;      // 中断次数加1
  if (cnt == 20)
  {          // 判断是否达到20次中断 (50ms * 20 = 1000ms = 1秒)
    cnt = 0; // 中断次数清零，为下一秒计数做准备
    // 如果计时未结束且未暂停
    if (time_num > 0 && !pause_flag)
    {
      time_num--; // 计时变量减1
    }
    // 如果计时结束
    if (time_num == 0)
    {
      TR0 = 0;        // 关闭定时器，停止计时
      alarm_flag = 1; // 设置报警标志位
    }
  }
}

/**
 * @brief  按键扫描函数
 * @note   检测P3.0-P3.3口的按键，并执行相应的操作
 */
void key_scan()
{
  // 清零键 (P3.0)
  if (P3_0 == 0)
  {
    delay(20); // 按键消抖延时
    if (P3_0 == 0)
    {                 // 再次确认按键是否真的按下
      time_num = 24;  // 时间重置为24秒
      pause_flag = 0; // 清除暂停标志
      alarm_flag = 0; // 清除报警标志
      TR0 = 0;        // 停止定时器
      P1_0 = 1;       // 关闭蜂鸣器
      while (P3_0 == 0)
        ; // 等待按键释放，防止重复触发
    }
  }
  // 启动键 (P3.1)
  if (P3_1 == 0)
  {
    delay(20);
    if (P3_1 == 0)
    {
      TR0 = 1; // 启动定时器，开始计时
      while (P3_1 == 0)
        ;
    }
  }
  // 暂停键 (P3.2)
  if (P3_2 == 0)
  {
    delay(20);
    if (P3_2 == 0)
    {
      pause_flag = 1; // 设置暂停标志
      while (P3_2 == 0)
        ;
    }
  }
  // 继续键 (P3.3)
  if (P3_3 == 0)
  {
    delay(20);
    if (P3_3 == 0)
    {
      pause_flag = 0; // 清除暂停标志，恢复计时
      while (P3_3 == 0)
        ;
    }
  }
}

/**
 * @brief  报警控制函数
 * @note   根据报警标志位控制P1.0口的蜂鸣器
 */
void alarm_ctrl()
{
  if (alarm_flag)
  {
    P1_0 = 0; // 报警标志位为1，P1.0输出低电平，蜂鸣器响
  }
  else
  {
    P1_0 = 1; // 报警标志位为0，P1.0输出高电平，蜂鸣器不响
  }
}

/**
 * @brief  主函数
 * @note   程序的入口和主循环
 */
void main(void)
{
  timer0_init(); // 初始化定时器0
  P1_0 = 1;      // 初始化P1.0口，关闭蜂鸣器

  while (1)
  {                    // 进入无限循环，使程序持续运行
    key_scan();        // 调用按键扫描函数
    display(time_num); // 调用显示函数
    alarm_ctrl();      // 调用报警控制函数
  }
}