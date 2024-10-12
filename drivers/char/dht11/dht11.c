/*************************************************************************
    > File Name: DHT11.c
    > 作者:YJK
    > Mail: 745506980@qq.com
    > Created Time: 2021年05月20日 星期四 21时33分51秒
 ************************************************************************/
#include<linux/init.h>
#include<linux/module.h>
#include <linux/platform_device.h>
#include <linux/of_gpio.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/delay.h>  //内核延时 忙等
#include <linux/uaccess.h>

#define ERROR_PRINT(x) do{	\
		printk(x " is error!\n"); \
		return -1; \
	}while(0)

#define DEVICE_NAME "dht11"

#define HIGH 1
#define LOW 0

/*1、platform匹配*/
/*2、匹配成功后，注册字符设备或杂项设备*/

int dht11_GPIO; //DHT11_GPIO编号

struct dht11_info {
	dev_t devt;
	struct cdev cdev;
	struct class *dht11_class;
	struct device *dht11_device;
};
struct dht11_info dht11;
/*起始信号
主机发送采集信号
主机由高电平-->低电平(至少18ms)--->高电平(20-40us)

DHT响应信号 低电平(80us)--->高电平(80us)

数据开始     低电平(50us)
数据0      高电平 26us-28us
数据1    	 高电平70us
*/

void end(void)
{
	gpio_direction_output(dht11_GPIO, HIGH);
//	gpio_set_value(dht11_GPIO, HIGH);
}
/*数据采集*/
unsigned char get_dht11_value(void)
{
	int i;
	unsigned char data = 0;
	for (i = 0; i < 8; i++) //8bit
	{
		if (gpio_get_value(dht11_GPIO) == LOW)
		{
			while(gpio_get_value(dht11_GPIO) == LOW); //低电平数据起始
			/*当变为高电平时, 等待40us 查看是否为高电平
				如果为高电平 则为 1
				如果为低电平 则为 0
			*/
		//	printk("LOW END\n");
			udelay(30);
			if (gpio_get_value(dht11_GPIO) == HIGH) //1
			{
				data |= (0x1 << (7 - i));  //数据自高位到低位
				udelay(40);
			}
			else
				data &= ~(0x1 << (7 - i)); // 置为0
		}
		else
			printk("------------ HIGH");
	}
	return data;
}

ssize_t dht11_read(struct file *file, char __user * user_buf, size_t size, loff_t * loff)
{
	unsigned char dht11_data[5];
	printk("start\n");
	ssleep(1);
	gpio_set_value(dht11_GPIO, LOW);
	mdelay(20); //拉低20ms
	/*拉高20-40us*/
	gpio_set_value(dht11_GPIO, HIGH);
	udelay(30); //拉高30us
	//设置为输入
	gpio_direction_input(dht11_GPIO);
	/*DHT11响应信号*/
	if (gpio_get_value(dht11_GPIO) == LOW)
	{
	//	printk("xiangying\n");
		//80us 低电平
		while(gpio_get_value(dht11_GPIO) == LOW);
		//80us 高电平
		while(gpio_get_value(dht11_GPIO) == HIGH);
		dht11_data[0] = get_dht11_value();	//湿度整数数据
		dht11_data[1] = get_dht11_value();	//湿度小数数据  0
		dht11_data[2] = get_dht11_value();	//温度整数数据
		dht11_data[3] = get_dht11_value();	//温度小数数据
		dht11_data[4] = get_dht11_value();	//校验和
		printk("data end\n");
	}
	else
		printk("start is error!\n");

	if (copy_to_user(user_buf, dht11_data, size) != 0)
	{
		ERROR_PRINT("copy_to_user");
	}
	printk("copy to user\n");
	//恢复为输出模式高电平
	end();
	return 0;
}
int dht11_open(struct inode * inode, struct file * file)
{
	return 0;
}
int dht11_release(struct inode * inode, struct file *file)
{
	return 0;
}
struct file_operations fops = {
	.open = dht11_open,
	.release = dht11_release,
	.read = dht11_read,
	.owner = THIS_MODULE
};

/*匹配成功进入probe函数*/
int dht11_probe(struct platform_device *pdev)
{
	/*匹配成功之后，获取硬件资源，然后注册字符设备*/
	/*SHT11-gpios = <&gpk3 0 GPIO_ACTIVE_HIGH>;*/
	int ret;
	printk("probe\n");
	dht11_GPIO = of_get_named_gpio(pdev->dev.of_node, "SHT11-gpios", 0);
	if (dht11_GPIO < 0)
	{
		ERROR_PRINT("of_get_named_gpio");
	}
	/*获取资源*/
	ret = gpio_request(dht11_GPIO, "dht11_gpio");
	if (ret != 0)
	{
		ERROR_PRINT("gpio_request!");
	}
	ret = gpio_direction_output(dht11_GPIO, HIGH);
	if (ret != 0)
	{
		ERROR_PRINT("gpio_direction_output");
	}
	/*注册字符设备驱动*/
	/*分配设备号*/
	ret = alloc_chrdev_region(&dht11.devt, 0, 1, "dht11");
	if (ret != 0)
	{
		ERROR_PRINT("alloc_chrdev_region");
	}
	/*初始化cdev*/
	dht11.cdev.owner = THIS_MODULE;
	cdev_init(&dht11.cdev, &fops);

	/*将cdev注册到内核*/
	ret = cdev_add(&dht11.cdev, dht11.devt,1);
	if (ret != 0)
	{
		ERROR_PRINT("cdev_add");
	}
	/*创建设备class*/
	dht11.dht11_class = class_create(THIS_MODULE, "dht11_class");
	if (dht11.dht11_class == NULL)
	{
		ERROR_PRINT("class_create");
	}
	/*在class下创建设备节点*/
	dht11.dht11_device = device_create(dht11.dht11_class, NULL, dht11.devt, NULL, DEVICE_NAME);
	if (dht11.dht11_device == NULL)
	{
		ERROR_PRINT("device_create");
	}
	printk("probe success!\n");
	return 0;
}
int dht11_remove(struct platform_device *pdev)
{
	return 0;
}
struct of_device_id of_match_table[] = {
	{.compatible = "dht11"},
	{}
}; //设备树匹配

struct platform_driver pdrv = {
	.probe = dht11_probe,
	.remove = dht11_remove,
	.driver = {
		.name = "dht11",
		.owner = THIS_MODULE,
		.of_match_table = of_match_table
	},
};
static int dht11_init(void)
{
	//注册platform_driver
	int ret = platform_driver_register(&pdrv);
	if (ret != 0)
		ERROR_PRINT("platform driver register");

	return 0;
}
static void dht11_exit(void)
{
	/*注销设备节点*/
	device_destroy(dht11.dht11_class, dht11.devt);
	/*注销class*/
	class_destroy(dht11.dht11_class);
	/*注销字符设备*/
	cdev_del(&dht11.cdev);
	/*注销设备号*/
	unregister_chrdev_region(dht11.devt, 1);
	/*注销GPIO*/
	gpio_free(dht11_GPIO);
	/*注销设备节点*/
	/*注销platform_driver*/
	platform_driver_unregister(&pdrv);
	printk("bye bye\n");
}

module_init(dht11_init);
module_exit(dht11_exit);

MODULE_LICENSE("GPL");

