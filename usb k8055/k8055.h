#include <usb.h>

class K8055 {
public:
	K8055(int devnr, bool debug);
	~K8055();
	int takeover_device( int interface );
	usb_dev_handle *locate_xsv(void);
	int write_empty_command ( unsigned char command );
	int write_dbt_command ( unsigned char command,unsigned char t1, unsigned char t2 );
	int write_output ( unsigned char a1, unsigned char a2, unsigned char d );
	int read_input ( unsigned char *a1, unsigned char *a2, unsigned char *d, unsigned short *c1, unsigned short *c2 );
	int msec_to_dbt_code ( int msec );
	int set_counter1_bouncetime ( int msec );
	int set_counter2_bouncetime ( int msec );
	int reset_counter1 ( );
	int reset_counter2 ( );
private:
	struct usb_device *cfg_devHandle;
	struct usb_dev_handle *xsv_handle;

	int debug ;

	int ipid;


};
