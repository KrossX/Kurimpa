char* CALLBACK PS2EgetLibName(void)
{
	return lib_name;
}

u32 CALLBACK PS2EgetLibType(void)
{
	return 0x01;
}

u32 CALLBACK PS2EgetLibVersion2(u32 type)
{
	return 0x06 << 16 | VERSION_MAJOR << 8 | VERSION_MINOR;
}
