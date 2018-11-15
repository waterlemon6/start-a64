#ifndef __JRUNNER_H__
#define __JRUNNER_H__

void JrunnerMain(const unsigned char *rbfData, int rbfLength);
void Configure(int device_count, int dev_seq, const unsigned char *rbfData, int rbfLength);
int CheckStatus(int device_count, int dev_seq);
void Startup(int device_count, int dev_seq);
void ProcessFileInput(const unsigned char *rbfData, int rbfLength);
void DriveSignal(int signal, int data, int clk);

#endif
