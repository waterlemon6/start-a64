#include "jb_io.h"
#include "jb_jtag.h"
#include "jb_const.h"
#include "jrunner.h"

int ji_info[MAX_DEVICE_ALLOW];

void JrunnerMain(const unsigned char *rbfData, int rbfLength)
{
    /**********Initialization**********/
    int i = 0;
    int device_count = 1;
    for(i = 0; i < MAX_DEVICE_ALLOW; i++)
        ji_info[i] = 10;

    /**********Device Chain Verification**********/
    Js_Reset();
    AdvanceJSM(0);
    /* Start configuration */
    for(i = 0; i < 3; i++)
    {
        Configure(device_count, 1, rbfData, rbfLength);
        if(!CheckStatus(device_count, 1))
            i=2;
    }
    Startup(device_count, 1);
}

void Configure(int device_count, int dev_seq, const unsigned char *rbfData, int rbfLength)
{
	int j = 0;

	/* Load PROGRAM instruction */
	SetupChain(device_count, dev_seq, ji_info, JI_PROGRAM);
    /* Drive TDI HIGH while moving JSM to SHIFTDR */
    DriveSignal(SIG_TDI, TDI_HIGH, TCK_QUIET);
    Js_Shiftdr();
    /* Start dumping configuration bits into TDI and clock with TCK */
    ProcessFileInput(rbfData, rbfLength);
    for(j = 0; j < 128; j++)
        DriveSignal(SIG_TDI, 1, TCK_TOGGLE);
    /* Move JSM to RUNIDLE */
    Js_Updatedr();
    Js_Runidle();
}

int CheckStatus(int device_count, int dev_seq)
{
	int bit, data = 0, error = 0;
	int conf_done_bit = 0;

	/* Load CHECK_STATUS instruction */
	SetupChain(device_count, dev_seq, ji_info, JI_CHECK_STATUS);
	Js_Shiftdr();
	conf_done_bit = 226;
	for(bit = 0; bit < conf_done_bit; bit++)
		DriveSignal(SIG_TDI, TDI_LOW, TCK_TOGGLE);
	data = ReadTDO(PORT_1, TDI_LOW, 0);
	if(!data)
		error++;
	/* Move JSM to RUNIDLE */
	Js_Updatedr();
	Js_Runidle();

	return (error);	
}

void Startup(int device_count, int dev_seq)
{
	int i;

	/* Load STARTUP instruction to move the device to USER mode */
	SetupChain(device_count, dev_seq, ji_info, JI_STARTUP);
	Js_Runidle();
	for(i = 0; i < INIT_COUNT; i++)
	{

		DriveSignal(SIG_TCK, TCK_LOW, TCK_QUIET);//1
		DriveSignal(SIG_TCK, TCK_HIGH, TCK_QUIET);//2
	}
	/* Reset JSM after the device is in USER mode */
	Js_Reset();
}

void ProcessFileInput(const unsigned char *rbfData, int rbfLength)
{
	unsigned char byte;
	int i = 0, j = 0;

	SET_TMS(((port_data[0] & 0xbe) & 0x2) >> 1);
	for(i = 0; i < rbfLength; i++)
	{
		byte = *(rbfData + i);
		/* Program a byte, from LSB to MSB */
		for (j = 0; j < 8; j++ )
		{
			SET_TCK(0);
			SET_TDI((byte >> j) & 0x1);
			SET_TCK(1);
		}
	}
}

void DriveSignal(int signal, int data, int clk)
{
	int mask;

	if(clk)
		mask = sig_port_maskbit[signal][1] | sig_port_maskbit[SIG_TCK][1];
	else
		mask = sig_port_maskbit[signal][1];
	mask = ~mask;
	port_data[0] = (unsigned char) ((port_data[0] & mask) | (data * sig_port_maskbit[signal][1]));

	SET_TCK((port_data[0] & 0x01));
	SET_TDI((port_data[0] & 0x40) >> 6);
	SET_TMS((port_data[0] & 0x02) >> 1);
	if(clk)
	{
		SET_TCK(((port_data[0] | sig_port_maskbit[SIG_TCK][1]) & 0x01));
		SET_TDI(((port_data[0] | sig_port_maskbit[SIG_TCK][1]) & 0x40) >> 6);
		SET_TMS(((port_data[0] | sig_port_maskbit[SIG_TCK][1]) & 0x02) >> 1);

		SET_TCK((port_data[0] & 0x01));
		SET_TDI((port_data[0] & 0x40) >> 6);
		SET_TMS((port_data[0] & 0x02) >> 1);
	}
}
