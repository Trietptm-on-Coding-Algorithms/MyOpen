#include <ntifs.h>

//�ṩһ��ж�غ������ó�����ж�أ����û���������������������ж�ء�
VOID UnDriver(PDRIVER_OBJECT driver)
{
	UNREFERENCED_PARAMETER(driver);
	KdPrint(("UnDriver.\n"));
}

//��ں������൱��main��
NTSTATUS DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING reg_path)
{
	UNREFERENCED_PARAMETER(driver);
	UNREFERENCED_PARAMETER(reg_path);
	KdPrint(("Hello World!\n"));
	driver->DriverUnload = UnDriver;
	return STATUS_SUCCESS;
}