
int mpc_i2c_write(int target, const u8 *data, int length, int restart, int offset);
int mpc_i2c_read(int target, u8 *data, int length, int restart, int offset);
int mpc_i2c_init(void);
