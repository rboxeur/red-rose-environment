@ stub NetAddAlternateComputerName
@ stub NetEnumerateComputerNames
@ stdcall NetGetJoinInformation( wstr ptr ptr ) netapi32.NetGetJoinInformation
@ stub NetGetJoinableOUs
@ stub NetJoinDomain
@ stub NetRemoveAlternateComputerName
@ stub NetRenameMachineInDomain
@ stub NetSetPrimaryComputerName
@ stub NetUnjoinDomain
@ stdcall NetUseAdd( str long ptr ptr ) netapi32.NetUseAdd
@ stdcall NetUseDel( str str long ) netapi32.NetUseDel
@ stdcall NetUseEnum( str long ptr long ptr ptr ptr ) netapi32.NetUseEnum
@ stdcall NetUseGetInfo( str str long ptr ) netapi32.NetUseGetInfo
@ stub NetValidateName
@ stdcall NetWkstaGetInfo( str long ptr ) netapi32.NetWkstaGetInfo
@ stub NetWkstaSetInfo
@ stub NetWkstaStatisticsGet
@ stub NetWkstaTransportAdd
@ stub NetWkstaTransportDel
@ stdcall NetWkstaTransportEnum( str long ptr long ptr ptr ptr ) netapi32.NetWkstaTransportEnum
@ stdcall NetWkstaUserEnum( str long ptr long ptr ptr ptr ) netapi32.NetWkstaUserEnum
@ stdcall NetWkstaUserGetInfo( str long ptr ) netapi32.NetWkstaUserGetInfo
@ stub NetWkstaUserSetInfo
