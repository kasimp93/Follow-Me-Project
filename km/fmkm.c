#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h> /* printk() */
#include <linux/slab.h> /* kmalloc() */
#include <linux/fs.h> /* everything... */
#include <linux/errno.h> /* error codes */
#include <linux/types.h> /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h> /* O_ACCMODE */
#include <asm/system.h> /* cli(), *_flags */
#include <asm/uaccess.h> /* copy_from/to_user */
#include <asm/param.h> /* include HZ */
#include <linux/string.h> /* string operations */
#include <linux/timer.h> /* timer gizmos */
#include <linux/list.h> /* include list data struct */
#include <asm/gpio.h>
#include <linux/interrupt.h>
#include <asm/arch/pxa-regs.h>
#include <asm-arm/arch/hardware.h>


MODULE_LICENSE("Dual BSD/GPL");

//static int fmkm_open(struct inode *inode, struct file *filp);
//static int fmkm_release(struct inode *inode, struct file *filp);
static ssize_t fmkm_read(struct file *filp,char *buf, size_t count, loff_t *f_pos);
static ssize_t fmkm_write(struct file *filp,const char *buf, size_t count, loff_t *f_pos);
static void timer_handler(unsigned long data);
static void fmkm_exit(void);
static int fmkm_init(void);
int ctrl[5] = {0,0,0,0,0};  // for stop//forward/backward/left/right 
int second = 1;

static char *msg;
static int msg_len;

//int operate_status = 0;   // control forward(1)/stop(0)/backward(2)
//int direction = 0;       // straight(0) left(1) right(2)
int time_count = 0;    // control how many cycles 
int speed = 0 ; // for pwm speed control

struct file_operations fmkm_fops = {
	read: fmkm_read,
	write: fmkm_write,
	//open: fmkm_open,
	//release: fmkm_release
};

module_init(fmkm_init);
module_exit(fmkm_exit);

static unsigned capacity = 2560;
static unsigned bite = 2560;
module_param(capacity, uint, S_IRUGO);
module_param(bite, uint, S_IRUGO);


static int fmkm_major = 61;


static char *fmkm_buffer;

static int fmkm_len;

static struct timer_list mytimer;


int mp[4]; // forward/backward/left/right 

static int fmkm_init(void)
{
	int result = 0;	
	/* Registering device */

	result = register_chrdev(fmkm_major, "fmkm", &fmkm_fops);

	
	if (result < 0){
		printk(KERN_ALERT
			"fmkm: cannot obtain major number %d\n", fmkm_major);
		return result;
	}

	fmkm_buffer = kmalloc(capacity, GFP_KERNEL);  // allocate buffer 
	
	if (!fmkm_buffer)
	{ 
		//printk(KERN_ALERT "Insufficient kernel memory\n"); 
		result = -ENOMEM;
		goto fail; 
	}
	
	memset(fmkm_buffer, 0, capacity);
	fmkm_len = 0;


	mp[0] = 28;
	mp[1] = 29;
	mp[2] = 17;
	mp[3] = 16;  // for pwm 
	
	gpio_direction_output(mp[0],1); 
	gpio_direction_output(mp[1],1);
	gpio_direction_output(mp[2],1); 
	gpio_direction_output(mp[3],1);
	
	pxa_gpio_set_value(mp[0],1); 
	pxa_gpio_set_value(mp[1],1);


	setup_timer(&mytimer, timer_handler, 0);      // setup new timer
	mytimer.expires=jiffies + (second*HZ);
	add_timer(&mytimer);

	/* initialize pwm stuff*/  
	pxa_set_cken(CKEN0_PWM0,1);  // set the clock
	pxa_gpio_mode(GPIO16_PWM0_MD);
	//PWM_CTRL0   = 0x00000020;
	PWM_PERVAL0 = 0xc8;
	PWM_PWDUTY0 = 0xfff;

		
	
	/* initialize pwm stuff*/
	pxa_set_cken(CKEN1_PWM1,1);  // set the clock
	pxa_gpio_mode(GPIO17_PWM1_MD);
	//PWM_CTRL0   = 0x00000020;
	PWM_PERVAL1 = 0xc8;
	PWM_PWDUTY1 = 0xfff;

	return 0;

fail:
	fmkm_exit();
	return result;
}

static void fmkm_exit(void)
{
	
	unregister_chrdev(fmkm_major, "fmkm");

	gpio_set_value(mp[0],1);
	gpio_set_value(mp[1],1);
	gpio_set_value(mp[2],1);
	gpio_set_value(mp[3],1);

	PWM_PWDUTY0 = 0xfff;
	PWM_PWDUTY1 = 0xfff;
	

	if (&mytimer)
		kfree(&mytimer);
	
	del_timer(&mytimer);
	
	printk(KERN_ALERT "Removing fmkm module\n");


}


//static int fmkm_open(struct inode *inode, struct file *filp)
//{
//	return 0;
//}

//static int fmkm_release(struct inode *inode, struct file *filp)
//{
	//return 0;
//}
/***********************************************

m0,1,2-- stop/forward/backward
d0,1,2,3-- stop/forward left/ forward right/ backward left / backward right 
v0,1,2-- speed high/medium/low



************************************************/

static ssize_t fmkm_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	char opcode,s_value;
	int i_value;
	int asc_value;
	printk(KERN_ALERT "It is writing\n");
	msg = kmalloc(128*sizeof(char), GFP_KERNEL);
	memset(msg, 0, 128);
	
	i_value = 0;
	if (copy_from_user(msg + *f_pos, buf, count))     // get the instruction and store it into msg
		return -EFAULT;

	opcode = msg[0];
	printk(KERN_ALERT "1:opcode is %c\n", opcode);
	s_value  = msg[1];	
	asc_value = (int)s_value;
	printk(KERN_ALERT "2:asc_value is %d\n", asc_value);

	if((s_value <= '9') && (s_value >= '0'))   // ASCII convert '1' to 1 , 1~9
		i_value = asc_value - 48;
	else if((s_value <= 'f') && (s_value >= 'a'))    // ASII convert 'a' to a , a~f
		i_value = asc_value - 87;

	if(opcode == 'm'){
			if(i_value == 1){   //forward
				ctrl[4] = 0;
				ctrl[3] = 1;
				ctrl[2] = 0;
				ctrl[1] = 0;
				ctrl[0] = 0;
			}
			else if (i_value == 2){  //backward
				ctrl[4] = 0;
				ctrl[3] = 0;
				ctrl[2] = 1;
				ctrl[1] = 0;
				ctrl[0] = 0;
			}	
			else if (i_value == 0){ // stop
				ctrl[4] = 1;
				ctrl[3] = 0;
				ctrl[2] = 0;
				ctrl[1] = 0;
				ctrl[0] = 0;
			}		
			time_count = 1;
	}
	else if(opcode == 'd'){    //0: stop 1:left forward 2: right forward 3: left backward 4:right backward 
			time_count = 1;
			if(i_value == 0){        //stop  1xx00
				ctrl[4] = 1;
				ctrl[3] = 0;
				ctrl[2] = 0;
				ctrl[1] = 0;
				ctrl[0] = 0;
			}
			else if (i_value ==1){   // left forward 0xx00
				ctrl[4] = 0;
				ctrl[3] = 0;
				ctrl[2] = 0;
				ctrl[1] = 0;
				ctrl[0] = 0;
			}
			else if (i_value == 2){  // right forward  0xx01
				ctrl[4] = 0;
				ctrl[3] = 0;
				ctrl[2] = 0;
				ctrl[1] = 0;
				ctrl[0] = 1;
			}
			else if (i_value == 3){  // left backward 0xx10
				ctrl[4] = 0;
				ctrl[3] = 0;
				ctrl[2] = 0;
				ctrl[1] = 1;
				ctrl[0] = 0;
			}
			else if (i_value == 4){   // right backward 0xx11
				ctrl[4] = 0;
				ctrl[3] = 0;
				ctrl[2] = 0;
				ctrl[1] = 1;
				ctrl[0] = 1;
			}
			
	}
	
	else if(opcode == 'v'){
		time_count = 1;
		speed = i_value;
		if((i_value >= 0) && (i_value <= 2)){
			switch(i_value){
		case 0:
			PWM_PWDUTY0 = 0x5;
			PWM_PWDUTY1 = 0x5;
			printk(KERN_ALERT
			"high\n");
			break;
		case 1: 
			PWM_PWDUTY0 = 0xa0;
			PWM_PWDUTY1 = 0xa0;
			printk(KERN_ALERT
			"medium\n");
			break;
		case 2:
			PWM_PWDUTY0 = 0xc7;
			PWM_PWDUTY1 = 0xc7;
			printk(KERN_ALERT
			"low\n");
			break;
		default:
			PWM_PWDUTY0 = 0xfff;
			PWM_PWDUTY0 = 0xfff;
		}
			
		printk(KERN_ALERT "Now the speed level is %d\n", i_value);
		}
	}
	else if (opcode == 'h'){
		gpio_direction_input(mp[2]);
	}

	*f_pos += count;
	msg_len = *f_pos;

	return count;
	
	
}

static ssize_t fmkm_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)	
{
	return 0;
}


void timer_handler(unsigned long data)
{
	int i;
	pxa_gpio_set_value(28,1);
	pxa_gpio_set_value(29,1);
	PWM_PWDUTY0 = 0xfff;
	PWM_PWDUTY1 = 0xfff;
	if(time_count > 0){
	if(ctrl[4] == 1){
		pxa_gpio_set_value(28,1);
		pxa_gpio_set_value(29,1);
		PWM_PWDUTY0 = 0xfff;
		PWM_PWDUTY1 = 0xfff;
	}
	else {
	if(ctrl[3] == 1){                      // forward
		if(speed == 0)
			PWM_PWDUTY0 = 0x5;
		else if(speed == 1)
			PWM_PWDUTY0 = 0xa0;
		else
			PWM_PWDUTY0 = 0xc7;
	}
	
       else if(ctrl[2] == 1){                 // backward
		if(speed == 0)
			PWM_PWDUTY1 = 0x5;
		else if(speed == 1)
			PWM_PWDUTY1 = 0xa0;
		else
			PWM_PWDUTY1 = 0xc7;
	}

	else if((ctrl[1] == 0) && (ctrl[0] == 0) ){  // left forward
		pxa_gpio_set_value(28,0);
		if(speed == 0)
			PWM_PWDUTY0 = 0x5;
		else if(speed == 1)
			PWM_PWDUTY0 = 0xa0;
		else
			PWM_PWDUTY0 = 0xc7;
	}
	else if((ctrl[1] == 0) && (ctrl[0] == 1)){  // right forward
		pxa_gpio_set_value(29,0);
		if(speed == 0)
			PWM_PWDUTY0 = 0x5;
		else if(speed == 1)
			PWM_PWDUTY0 = 0xa0;
		else
			PWM_PWDUTY0 = 0xc7;
	}
	else if((ctrl[1] == 1) && (ctrl[0] == 0)){  // left backward
		pxa_gpio_set_value(28,0);
		if(speed == 0)
			PWM_PWDUTY1 = 0x5;
		else if(speed == 1)
			PWM_PWDUTY1 = 0xa0;
		else
			PWM_PWDUTY1 = 0xc7;
	}
	else if((ctrl[1] == 1) && (ctrl[0] == 1)){  // right backward
		pxa_gpio_set_value(29,0);
		if(speed == 0)
			PWM_PWDUTY1 = 0x5;
		else if(speed == 1)
			PWM_PWDUTY1 = 0xa0;
		else
			PWM_PWDUTY1 = 0xc7;
	}
	
	else{
		pxa_gpio_set_value(28,1);
		pxa_gpio_set_value(29,1);
		PWM_PWDUTY0 = 0xfff;
		PWM_PWDUTY1 = 0xfff;
	}
	}
	}
	for(i = 0; i<=4 ; i++)  // reset all the control signals
		ctrl[i] = 0;
	time_count --;
	mod_timer(&mytimer,jiffies + msecs_to_jiffies(200*second));
	
		

}



