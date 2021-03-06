/*
 * Copyright 2005 - 2015  Zarafa B.V. and its licensors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License, version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef ECGUID_H
#define ECGUID_H

////////////////////////////////////////////////////////////////////////////
// Zarafa Internal used guids

// MAPI Interface implementations

// {8956AF38-EA4F-42bf-B24F-D4D8B6196A4A}
DEFINE_GUID(IID_ECMSProvider, 
0x8956af38, 0xea4f, 0x42bf, 0xb2, 0x4f, 0xd4, 0xd8, 0xb6, 0x19, 0x6a, 0x4a);

// {E3B26C47-AB5D-4586-97F3-AEDE80474136}
DEFINE_GUID(IID_ECXPProvider, 
0xe3b26c47, 0xab5d, 0x4586, 0x97, 0xf3, 0xae, 0xde, 0x80, 0x47, 0x41, 0x36);

// {2D3C584B-718D-410e-A383-73D4F74978DC}
DEFINE_GUID(IID_ECABProvider, 
0x2d3c584b, 0x718d, 0x410e, 0xa3, 0x83, 0x73, 0xd4, 0xf7, 0x49, 0x78, 0xdc);

// {766D15B9-23BE-41be-AAD2-E85D0849F5A2}
DEFINE_GUID(IID_ECMsgStore, 
0x766d15b9, 0x23be, 0x41be, 0xaa, 0xd2, 0xe8, 0x5d, 0x8, 0x49, 0xf5, 0xa2);

// {0999C37B-9F73-42bb-BA57-B88940FDD686}
DEFINE_GUID(IID_ECMSLogon, 
0x999c37b, 0x9f73, 0x42bb, 0xba, 0x57, 0xb8, 0x89, 0x40, 0xfd, 0xd6, 0x86);

// {432ADE55-91F5-4669-AEE5-D45B1F1A77DF}
DEFINE_GUID(IID_ECXPLogon, 
0x432ade55, 0x91f5, 0x4669, 0xae, 0xe5, 0xd4, 0x5b, 0x1f, 0x1a, 0x77, 0xdf);

// {19249E3B-D2E5-4216-BB64-180A7D6DC430}
DEFINE_GUID(IID_ECABLogon, 
0x19249e3b, 0xd2e5, 0x4216, 0xbb, 0x64, 0x18, 0xa, 0x7d, 0x6d, 0xc4, 0x30);

// {EDBFF82B-E110-461d-AAD1-338FD32E2587}
DEFINE_GUID(IID_ECMAPIFolder, 
0xedbff82b, 0xe110, 0x461d, 0xaa, 0xd1, 0x33, 0x8f, 0xd3, 0x2e, 0x25, 0x87);

// {E7B4AEFB-30E9-4f69-B4D4-E19744F24B5B}
DEFINE_GUID(IID_ECMAPIFolderPublic, 
0xe7b4aefb, 0x30e9, 0x4f69, 0xb4, 0xd4, 0xe1, 0x97, 0x44, 0xf2, 0x4b, 0x5b);

// {F88FAD0A-2A80-4bf6-9336-302CF7B8E2E2}
DEFINE_GUID(IID_ECMessage, 
0xf88fad0a, 0x2a80, 0x4bf6, 0x93, 0x36, 0x30, 0x2c, 0xf7, 0xb8, 0xe2, 0xe2);

// {41DEDE42-5481-4027-B379-C69E879FFD34}
DEFINE_GUID(IID_ECMAPIProp, 
0x41dede42, 0x5481, 0x4027, 0xb3, 0x79, 0xc6, 0x9e, 0x87, 0x9f, 0xfd, 0x34);

// {A7CCBF57-CAA9-4167-89BD-F2A791B6EB8D}
DEFINE_GUID(IID_ECMAPITable, 
0xa7ccbf57, 0xcaa9, 0x4167, 0x89, 0xbd, 0xf2, 0xa7, 0x91, 0xb6, 0xeb, 0x8d);

// {D78DD32E-C3A7-4e4d-AA79-8B0F28F9DA74}
DEFINE_GUID(IID_ECRecipTable, 
0xd78dd32e, 0xc3a7, 0x4e4d, 0xaa, 0x79, 0x8b, 0xf, 0x28, 0xf9, 0xda, 0x74);

// {B6AC9507-3345-4456-AE53-F0153629120A}
DEFINE_GUID(IID_ECMAPIContainer, 
0xb6ac9507, 0x3345, 0x4456, 0xae, 0x53, 0xf0, 0x15, 0x36, 0x29, 0x12, 0xa);

// {0AF6A8E8-FC28-4b66-88D0-644062C97F56}
DEFINE_GUID(IID_ECABContainer, 
0xaf6a8e8, 0xfc28, 0x4b66, 0x88, 0xd0, 0x64, 0x40, 0x62, 0xc9, 0x7f, 0x56);

// {3E76319E-763C-4027-9A0C-60A8B9C07538}
DEFINE_GUID(IID_ECExchangeImportHierarchyChanges, 
0x3e76319e, 0x763c, 0x4027, 0x9a, 0xc, 0x60, 0xa8, 0xb9, 0xc0, 0x75, 0x38);

// {3702C93D-69E2-40f6-86F1-13444C3DA20D}
DEFINE_GUID(IID_ECExchangeImportContentsChanges, 
0x3702c93d, 0x69e2, 0x40f6, 0x86, 0xf1, 0x13, 0x44, 0x4c, 0x3d, 0xa2, 0xd);

// {384DD97E-FFD4-4577-9B99-D6F9779B19DF}
DEFINE_GUID(IID_ECExchangeExportChanges, 
0x384dd97e, 0xffd4, 0x4577, 0x9b, 0x99, 0xd6, 0xf9, 0x77, 0x9b, 0x19, 0xdf);


// Internals

// {89A63BAC-97DA-4425-8665-695611CA4F53}
DEFINE_GUID(IID_ECTableOutGoingQueue, 
0x89a63bac, 0x97da, 0x4425, 0x86, 0x65, 0x69, 0x56, 0x11, 0xca, 0x4f, 0x53);

// {4DFE39BD-1DA7-47e1-A572-E1D266883D4E}
DEFINE_GUID(IID_ECNotifyClient, 
0x4dfe39bd, 0x1da7, 0x47e1, 0xa5, 0x72, 0xe1, 0xd2, 0x66, 0x88, 0x3d, 0x4e);

// {475300CD-F83F-4b55-87FB-4A59C11A6395}
DEFINE_GUID(IID_IECPropStorage, 
0x475300cd, 0xf83f, 0x4b55, 0x87, 0xfb, 0x4a, 0x59, 0xc1, 0x1a, 0x63, 0x95);

// {4FB1E905-7317-4560-A3BC-B544ABFDBF78}
DEFINE_GUID(IID_ECTransport, 
0x4fb1e905, 0x7317, 0x4560, 0xa3, 0xbc, 0xb5, 0x44, 0xab, 0xfd, 0xbf, 0x78);

// {B99F216D-4F02-4492-AC48-E40379C5F7EC}
DEFINE_GUID(IID_ECMAPIFolderOps, 
0xb99f216d, 0x4f02, 0x4492, 0xac, 0x48, 0xe4, 0x3, 0x79, 0xc5, 0xf7, 0xec);

// {6510468F-6794-493b-8F0A-A7E02A904CD5}
DEFINE_GUID(IID_ECTableView, 
0x6510468f, 0x6794, 0x493b, 0x8f, 0xa, 0xa7, 0xe0, 0x2a, 0x90, 0x4c, 0xd5);

// {42F691D7-F7BB-4646-94AB-A0C9652F9904}
DEFINE_GUID(IDD_ECUserGroupArray, 
0x42f691d7, 0xf7bb, 0x4646, 0x94, 0xab, 0xa0, 0xc9, 0x65, 0x2f, 0x99, 0x4);

// {F3022F76-9DE9-4d8b-AE05-B3F3B71B39B6}
DEFINE_GUID(IDD_ECRightsArray, 
0xf3022f76, 0x9de9, 0x4d8b, 0xae, 0x5, 0xb3, 0xf3, 0xb7, 0x1b, 0x39, 0xb6);

// {CC82DA63-A36F-4de9-840A-2DEAFC56BF31}
DEFINE_GUID(IID_ECTransportNotify, 
0xcc82da63, 0xa36f, 0x4de9, 0x84, 0xa, 0x2d, 0xea, 0xfc, 0x56, 0xbf, 0x31);

// {A7712BD8-A76A-4bd2-BCFC-5BE4AFC3BBE5}
DEFINE_GUID(IID_ECAttach, 
0xa7712bd8, 0xa76a, 0x4bd2, 0xbc, 0xfc, 0x5b, 0xe4, 0xaf, 0xc3, 0xbb, 0xe5);

// {AD68DBFB-F741-48fc-AEA1-A52C895510DD}
DEFINE_GUID(IID_ECMemPropStorage, 
0xad68dbfb, 0xf741, 0x48fc, 0xae, 0xa1, 0xa5, 0x2c, 0x89, 0x55, 0x10, 0xdd);

DEFINE_GUID(GUID_NULL, 
0x00000000, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

// {516c1130-c316-41b9-ad66-66f111d3592e}
DEFINE_GUID(IID_ECChunkStorage,
0x516c1130, 0xc316, 0x41b9, 0xad, 0x66, 0x66, 0xf1, 0x11, 0xd3, 0x59, 0x2e);

// {38E50177-30C8-4081-8DD5-9E0B5BC7E7EF}
DEFINE_GUID(IID_ECMAPISupport, 
0x38e50177, 0x30c8, 0x4081, 0x8d, 0xd5, 0x9e, 0xb, 0x5b, 0xc7, 0xe7, 0xef);

// {373593A9-2B97-4c8b-8EBF-EB6AB0BA960F}
DEFINE_GUID(IID_WSMAPIPropStorage, 
0x373593a9, 0x2b97, 0x4c8b, 0x8e, 0xbf, 0xeb, 0x6a, 0xb0, 0xba, 0x96, 0xf);

// {71080DA1-F54C-4a6d-8A64-31CF155B7F5A}
DEFINE_GUID(IID_ECMAPIOfflineMgr, 
0x71080da1, 0xf54c, 0x4a6d, 0x8a, 0x64, 0x31, 0xcf, 0x15, 0x5b, 0x7f, 0x5a);

DEFINE_GUID(IID_ECParentStorage,
0x4d404cfb, 0xfdd4, 0x4c71, 0x84,0xf1, 0xe2, 0xd5, 0xea, 0xd4, 0xb8, 0x31);

// {DE3598D9-02B4-449d-8116-27D978C4587F}
DEFINE_GUID(IID_WSABPropStorage, 
0xde3598d9, 0x2b4, 0x449d, 0x81, 0x16, 0x27, 0xd9, 0x78, 0xc4, 0x58, 0x7f);

// {77B5240D-8611-44a9-A4D3-C88CF7033D5E}
DEFINE_GUID(IID_ECABProp, 
0x77b5240d, 0x8611, 0x44a9, 0xa4, 0xd3, 0xc8, 0x8c, 0xf7, 0x3, 0x3d, 0x5e);

// {359DD775-AC2A-4fec-AE42-7FC2E870D4D0}
DEFINE_GUID(IID_ECMailUser, 
0x359dd775, 0xac2a, 0x4fec, 0xae, 0x42, 0x7f, 0xc2, 0xe8, 0x70, 0xd4, 0xd0);

// {1112A78F-84FB-4539-A373-6A4967E72068}
DEFINE_GUID(IID_ECDistList, 
0x1112a78f, 0x84fb, 0x4539, 0xa3, 0x73, 0x6a, 0x49, 0x67, 0xe7, 0x20, 0x68);

// Same as MUIDECSAB_SERVER
// {50A921AC-D340-48ee-B319-FBA753304425}
DEFINE_GUID(MUIDECSAB, 
0x50a921ac, 0xd340, 0x48ee, 0xb3, 0x19, 0xfb, 0xa7, 0x53, 0x30, 0x44, 0x25);

////////////////////////////////////////////////////////////////////////////
// Zarafa external used guids

// {3C253DCA-D227-443c-94FE-425FAB958C19}
DEFINE_GUID(ZARAFA_SERVICE_GUID, 
0x3c253dca, 0xd227, 0x443c, 0x94, 0xfe, 0x42, 0x5f, 0xab, 0x95, 0x8c, 0x19);

// {D47F4A09-D3BD-493c-B2FC-3C90BBCB48D4}
DEFINE_GUID(ZARAFA_STORE_PUBLIC_GUID, 
0xd47f4a09, 0xd3bd, 0x493c, 0xb2, 0xfc, 0x3c, 0x90, 0xbb, 0xcb, 0x48, 0xd4);

// {7C7C1085-BC6D-4e53-9DAB-8A53F8DEF808}
DEFINE_GUID(ZARAFA_STORE_DELEGATE_GUID, 
0x7c7c1085, 0xbc6d, 0x4e53, 0x9d, 0xab, 0x8a, 0x53, 0xf8, 0xde, 0xf8, 0x8);

// {BC8953AD-2E3F-4172-9404-896FF459870F}
DEFINE_GUID(ZARAFA_STORE_ARCHIVE_GUID,
0xbc8953ad, 0x2e3f, 0x4172, 0x94, 0x4, 0x89, 0x6f, 0xf4, 0x59, 0x87, 0xf);

// {5ABAE40E-1DF7-4f3d-964D-658A79924E9D}
DEFINE_GUID(ZARAFA_SAB_GUID, 
0x5abae40e, 0x1df7, 0x4f3d, 0x96, 0x4d, 0x65, 0x8a, 0x79, 0x92, 0x4e, 0x9d);

// {7976ED54-D0D2-11DD-9705-BE5055D89593}
#define MUIDECSI_SERVER "\x79\x76\xED\x54\xD0\xD2\x11\xDD\x97\x05\xBE\x50\x55\xD8\x95\x93"

// {A7D80ED6-D027-11DD-B0B6-645055D89593}
DEFINE_GUID(IID_IECSingleInstance,
0xa7d80ed6, 0xd027, 0x11dd, 0xb0, 0xb6, 0x64, 0x50, 0x55, 0xd8, 0x95, 0x93);

// {20C5963F-0E0B-4d7f-B75D-8ACD88727119}
DEFINE_GUID(IID_IECSpooler, 
0x20c5963f, 0xe0b, 0x4d7f, 0xb7, 0x5d, 0x8a, 0xcd, 0x88, 0x72, 0x71, 0x19);

// {A4445398-6996-4a29-8C45-50C83E3DE270}
DEFINE_GUID(IID_IECServiceAdmin, 
0xa4445398, 0x6996, 0x4a29, 0x8c, 0x45, 0x50, 0xc8, 0x3e, 0x3d, 0xe2, 0x70);

// {6D4FE98E-09DF-4d85-A191-896B1F89F911}
DEFINE_GUID(IID_IECSecurity, 
0x6d4fe98e, 0x9df, 0x4d85, 0xa1, 0x91, 0x89, 0x6b, 0x1f, 0x89, 0xf9, 0x11);

// {B1514D30-D8BF-48A3-9560-6F78E7A5005B}
DEFINE_GUID(IID_IECMultiStoreTable,
0xb1514d30, 0xd8bf, 0x48a3, 0x95, 0x60, 0x6f, 0x78, 0xe7, 0xa5, 0x00, 0x5b);

// {a9a55c9a-d2bb-461e-9ff5-fd4ed9844c2e}
DEFINE_GUID(IID_IECLicense,
0xa9a55c9a, 0xd2bb, 0x461e, 0x9f, 0xf5, 0xfd, 0x4e, 0xd9, 0x84, 0x4c, 0x2e);

// {49941EA5-E4BC-46c5-BF61-E94C1543B612}
DEFINE_GUID(IID_ECMemBlock, 
0x49941ea5, 0xe4bc, 0x46c5, 0xbf, 0x61, 0xe9, 0x4c, 0x15, 0x43, 0xb6, 0x12);

// {6C5377B7-B5CA-4e14-BC94-E16048BA2B10}
DEFINE_GUID(IID_ECMemStream, 
0x6c5377b7, 0xb5ca, 0x4e14, 0xbc, 0x94, 0xe1, 0x60, 0x48, 0xba, 0x2b, 0x10);

// {1A2038D1-4152-42b5-90C5-D4D6126B9314}
DEFINE_GUID(IID_ECUnknown, 
0x1a2038d1, 0x4152, 0x42b5, 0x90, 0xc5, 0xd4, 0xd6, 0x12, 0x6b, 0x93, 0x14);

// {707E2DC8-CF4D-438e-B220-2F17AA792DD1}
DEFINE_GUID(IID_ECMemTable,
0x707e2dc8, 0xcf4d, 0x438e, 0xb2, 0x20, 0x2f, 0x17, 0xaa, 0x79, 0x2d, 0xd1);

// {75C99339-15B4-4d9a-B384-69ED7AD5479E}
DEFINE_GUID(IID_ECMemTablePublic, 
0x75c99339, 0x15b4, 0x4d9a, 0xb3, 0x84, 0x69, 0xed, 0x7a, 0xd5, 0x47, 0x9e);

// {68E696C9-F905-4ff4-B677-2C951047D0EE}
DEFINE_GUID(IID_ECMemTableView, 
0x68e696c9, 0xf905, 0x4ff4, 0xb6, 0x77, 0x2c, 0x95, 0x10, 0x47, 0xd0, 0xee);

// {BD574D55-174E-4873-9873-579EB192243C}
DEFINE_GUID(IID_ECExchangeModifyTable, 
0xbd574d55, 0x174e, 0x4873, 0x98, 0x73, 0x57, 0x9e, 0xb1, 0x92, 0x24, 0x3c);

// {3F1CFEF4-52A3-4FC4-B196-34C76483FCD8}
DEFINE_GUID(IID_IECExchangeModifyTable, 
0x3f1cfef4, 0x52a3, 0x4fc4, 0xb1, 0x96, 0x34, 0xc7, 0x64, 0x83, 0xfc, 0xd8);

// {6603936A-3AFC-4276-BD91-0C543615C8FF}
DEFINE_GUID(IID_ECFreeBusySupport, 
0x6603936a, 0x3afc, 0x4276, 0xbd, 0x91, 0xc, 0x54, 0x36, 0x15, 0xc8, 0xff);

// {EE4E17E9-22CA-47e1-8A60-06074BAD1FD3}
DEFINE_GUID(IID_ECFreeBusyData, 
0xee4e17e9, 0x22ca, 0x47e1, 0x8a, 0x60, 0x6, 0x7, 0x4b, 0xad, 0x1f, 0xd3);

// {F1E1CA2C-4FB9-48fa-9D3C-B22AFD51879F}
DEFINE_GUID(IID_ECEnumFBBlock, 
0xf1e1ca2c, 0x4fb9, 0x48fa, 0x9d, 0x3c, 0xb2, 0x2a, 0xfd, 0x51, 0x87, 0x9f);

// {C60B88A3-17C8-4206-B8A0-C68C09841330}
DEFINE_GUID(IID_ECFreeBusyUpdate, 
0xc60b88a3, 0x17c8, 0x4206, 0xb8, 0xa0, 0xc6, 0x8c, 0x9, 0x84, 0x13, 0x30);

// {a2656166-eaee-4cf7-8c55-4c05b66b81bb}
DEFINE_GUID(IID_IECExportChanges,
0xa2656166, 0xeaee, 0x4cf7, 0x8c, 0x55, 0x4c, 0x05, 0xb6, 0x6b, 0x81, 0xbb);


// {13CC44F2-B516-4a03-B2C0-F33B47CADF89}
DEFINE_GUID(IID_ECMsgStoreOffline, 
0x13cc44f2, 0xb516, 0x4a03, 0xb2, 0xc0, 0xf3, 0x3b, 0x47, 0xca, 0xdf, 0x89);

// {013E8536-131E-44e5-A46A-F399B9C05391}
DEFINE_GUID(IID_ECMsgStoreOnline, 
0x13e8536, 0x131e, 0x44e5, 0xa4, 0x6a, 0xf3, 0x99, 0xb9, 0xc0, 0x53, 0x91);

// {09f33e65-6886-4bf2-a26f-fa0ec4e94216}
DEFINE_GUID(IID_IECExportAddressbookChanges,
0x09f33e65, 0x6886, 0x4bf2, 0xa2, 0x6f, 0xfa, 0x0e, 0xc4, 0xe9, 0x42, 0x16);

// {1843c89e-2411-11de-be6e-7c3256d89593}
DEFINE_GUID(IID_IECImportAddressbookChanges,
0x1843c89e, 0x2411, 0x11de, 0xbe, 0x6e, 0x7c, 0x32, 0x56, 0xd8, 0x95, 0x93);

// {1c2d9897-f459-4a55-9d11-cd9e7c7551ee}
DEFINE_GUID(IID_IECServerBehavior,
0x1c2d9897, 0xf459, 0x4a55, 0x9d, 0x11, 0xcd, 0x9e, 0x7c, 0x75, 0x51, 0xee);

// {7175DE83-33A2-4d69-8D9A-17726EB2FCDD}
DEFINE_GUID(IID_IECServerMimic, 
0x7175de83, 0x33a2, 0x4d69, 0x8d, 0x9a, 0x17, 0x72, 0x6e, 0xb2, 0xfc, 0xdd);

//FIXME: Move those guids non zarafa guids, only linux?

// {00020400-0000-0000-C000-000000000046}
DEFINE_GUID(IID_IDispatch,
0x00020400, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

// {d597bab1-5b9f-11d1-8dd2-00aa004abd5e}
DEFINE_GUID(IID_ISensNetwork,
0xd597bab1, 0x5b9f, 0x11d1, 0x8d, 0xd2, 0x00, 0xaa, 0x00, 0x4a, 0xbd, 0x5e);

// {49A1CE2F-DE5A-40ec-BBAF-7ED64BDF8889}
DEFINE_GUID(IID_IECChangeAdvisor, 
0x49a1ce2f, 0xde5a, 0x40ec, 0xbb, 0xaf, 0x7e, 0xd6, 0x4b, 0xdf, 0x88, 0x89);

// {B767BD7C-EBF1-4d86-9112-601C8D9D7D9F}
DEFINE_GUID(IID_ECChangeAdvisor, 
0xb767bd7c, 0xebf1, 0x4d86, 0x91, 0x12, 0x60, 0x1c, 0x8d, 0x9d, 0x7d, 0x9f);

// {C71DE317-16E4-43a8-963E-6F06F4462108}
DEFINE_GUID(IID_IECChangeAdviseSink, 
0xc71de317, 0x16e4, 0x43a8, 0x96, 0x3e, 0x6f, 0x6, 0xf4, 0x46, 0x21, 0x8);

// {C6937B5A-48D7-4017-A041-2563590B1F5F}
DEFINE_GUID(IID_ECChangeAdviseSink, 
0xc6937b5a, 0x48d7, 0x4017, 0xa0, 0x41, 0x25, 0x63, 0x59, 0xb, 0x1f, 0x5f);

// {2458600A-F010-4ee4-B8E1-BFD7C1AA1EC5}
DEFINE_GUID(IID_IECImportContentsChanges, 
0x2458600a, 0xf010, 0x4ee4, 0xb8, 0xe1, 0xbf, 0xd7, 0xc1, 0xaa, 0x1e, 0xc5);

// {EA6F84AF-9326-4C67-B453-5AEC477CEC88}
DEFINE_GUID(IID_IECImportHierarchyChanges,
0xea6f84af, 0x9326, 0x4c67, 0xb4, 0x53, 0x5a, 0xec, 0x47, 0x7c, 0xec, 0x88);

// {74C79593-1320-404c-AE79-607A9016F3F5}
DEFINE_GUID(IID_IECSync, 
0x74c79593, 0x1320, 0x404c, 0xae, 0x79, 0x60, 0x7a, 0x90, 0x16, 0xf3, 0xf5);

// {8F6CB9B3-0406-4a7a-B8BA-95372001D413}
DEFINE_GUID(IID_IECTestProtocol, 
0x8f6cb9b3, 0x406, 0x4a7a, 0xb8, 0xba, 0x95, 0x37, 0x20, 0x1, 0xd4, 0x13);

// {54CD0028-46E7-4555-B68F-1AE79D6171C1}
DEFINE_GUID(IID_IECMessageRaw, 
0x54cd0028, 0x46e7, 0x4555, 0xb6, 0x8f, 0x1a, 0xe7, 0x9d, 0x61, 0x71, 0xc1);

// Unique identifier of a public entryid
#define STATIC_GUID_PUBLICFOLDER "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x03"
#define STATIC_GUID_FAVORITE "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x02"
#define STATIC_GUID_FAVSUBTREE "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01"

/////////////////////////////////////////
// ECprops guid
//

// {DC1182DC-9A94-461f-88D0-C4A861D6D8CC}
DEFINE_GUID(IID_ECExplorerEventSink, 
0xdc1182dc, 0x9a94, 0x461f, 0x88, 0xd0, 0xc4, 0xa8, 0x61, 0xd6, 0xd8, 0xcc);

// {DCE7C05E-AA2A-43ac-8E19-967796FAB1AF}
DEFINE_GUID(IID_ECSyncBar, 
0xdce7c05e, 0xaa2a, 0x43ac, 0x8e, 0x19, 0x96, 0x77, 0x96, 0xfa, 0xb1, 0xaf);

// {C6DAFA47-168C-4a3e-9C0C-2650CA48C7CA}
DEFINE_GUID(IID_ECCommandBarButton, 
0xc6dafa47, 0x168c, 0x4a3e, 0x9c, 0x0c, 0x26, 0x50, 0xca, 0x48, 0xc7, 0xca);

/////////////////////////////////////////
// Named property guids
//

// {00F5F108-8E3F-46C7-AF72-5E201C2349E7}
DEFINE_GUID(PS_EC_IMAP,
0x00f5f108, 0x8e3f, 0x46c7, 0xaf, 0x72, 0x5e, 0x20, 0x1c, 0x23, 0x49, 0xe7);


/////////////////////////////////////////
// Zarafa Contacts provider guids
//

// {727F0430-E392-4FDA-B86A-E52A7FE46571}, provider
DEFINE_GUID(MUIDZCSAB,
0x30047f72, 0x92e3, 0xda4f, 0xb8, 0x6a, 0xE5, 0x2A, 0x7F, 0xE4, 0x65, 0x71);

DEFINE_GUID(IID_ZCDistList,
0x85bd8924, 0x879b, 0x4344, 0x94, 0xcc, 0x7d, 0xf7, 0x40, 0x6d, 0x1d, 0xaf);

DEFINE_GUID(IID_ZCMAPIProp,
0xa2b4a37f, 0xf99d, 0x4ee0, 0xa9, 0x04, 0x9b, 0x11, 0xae, 0x16, 0x55, 0x27);

// {DC73D71E-6EC9-442F-B18A-64D196B50753}
DEFINE_GUID(IID_ZCABProvider,
0xdc73d71e, 0x6ec9, 0x442f, 0xb1, 0x8a, 0x64, 0xd1, 0x96, 0xb5, 0x07, 0x53);

// {270D247B-7235-47F7-9AAE-66BA28B55D56}
DEFINE_GUID(IID_ZCABLogon, 
0x270d247b, 0x7235, 0x47f7, 0x9a, 0xae, 0x66, 0xba, 0x28, 0xb5, 0x5d, 0x56);

// {70954acf-5f32-4089-9e2d-7ea51885b9e4}
DEFINE_GUID(IID_ZCABContainer,
0x70954acf, 0x5f32, 0x4089, 0x9e, 0x2d, 0x7e, 0xa5, 0x18, 0x85, 0xb9, 0xe4);

/////////////////////////////////////////
// libcalendar guids (should those even be here?)
//

DEFINE_GUID(IID_Appointment,
0x4eabe417, 0x4400, 0x4025, 0x85, 0xfb, 0xcf, 0xf3, 0xac, 0x55, 0xd1, 0x70);

// {1dcbe4b6-e8cb-4d20-aca2-29cbdb646ace}
DEFINE_GUID(IID_IAppointment,
0x1dcbe4b6, 0xe8cb, 0x4d20, 0xac, 0xa2, 0x29, 0xcb, 0xdb, 0x64, 0x6a, 0xce);

// {51ab2ebc-25fb-41e8-b67d-fb572ac4e478}
DEFINE_GUID(IID_IOccurrence,
0x51ab2ebc, 0x25fb, 0x41e8, 0xb6, 0x7d, 0xfb, 0x57, 0x2a, 0xc4, 0xe4, 0x78);

// {4daea497-fece-4a06-9233-c043d93ff9fa}
DEFINE_GUID(IID_IRecurrencePatternInspector,
0x4daea497, 0xfece, 0x4a06, 0x92, 0x33, 0xc0, 0x43, 0xd9, 0x3f, 0xf9, 0xfa);

// {0ec9cc83-ec6e-4ee5-91b7-cf35a724adc2}
DEFINE_GUID(IID_MeetingCancellation,
0x0ec9cc83, 0xec6e, 0x4ee5, 0x91, 0xb7, 0xcf, 0x35, 0xa7, 0x24, 0xad, 0xc2);

// {af0320a4-d726-4011-8cde-3424a90cef97}
DEFINE_GUID(IID_IMeetingCancellation,
0xaf0320a4, 0xd726, 0x4011, 0x8c, 0xde, 0x34, 0x24, 0xa9, 0x0c, 0xef, 0x97);

// {fffa6f83-ff2f-4a7b-819b-f7ec1c180f36}
DEFINE_GUID(IID_MeetingRequest,
0xfffa6f83, 0xff2f, 0x4a7b, 0x81, 0x9b, 0xf7, 0xec, 0x1c, 0x18, 0x0f, 0x36);

// {018d61b8-65b1-42dd-ae37-9d7a4f8ea2db}
DEFINE_GUID(IID_IMeetingRequest,
0x018d61b8, 0x65b1, 0x42dd, 0xae, 0x37, 0x9d, 0x7a, 0x4f, 0x8e, 0xa2, 0xdb);

// {93afcbba-4f5a-4aac-801a-ccb995d27f36}
DEFINE_GUID(IID_MeetingResponse,
0x93afcbba, 0x4f5a, 0x4aac, 0x80, 0x1a, 0xcc, 0xb9, 0x95, 0xd2, 0x7f, 0x36);

// {034ccbe9-52a5-42d0-80e9-3185ef0f051d}
DEFINE_GUID(IID_IMeetingResponse,
0x034ccbe9, 0x52a5, 0x42d0, 0x80, 0xe9, 0x31, 0x85, 0xef, 0x0f, 0x05, 0x1d);

#endif

