#include <string.h>
#include "runtime/btdev.h"
#include "services/bt.h"
#include "services/btmu.h"

Result btdevInitialize(void) {
    Result rc=0;

    rc = btInitialize();
    if (R_SUCCEEDED(rc)) {
        rc = btmuInitialize();
        if (R_FAILED(rc)) btExit();
    }
    return rc;
}

void btdevExit(void) {
    btmuExit();
    btExit();
}

// sdknso does Result error-conversion, but we won't do so.

bool btdevGattAttributeUuidIsSame(const BtdrvGattAttributeUuid *a, const BtdrvGattAttributeUuid *b) {
    if (a->size != b->size) return false;
    if (a->size != 0x2 && a->size != 0x4 && a->size != 0x10) return false;
    return memcmp(a->uuid, b->uuid, a->size)==0;
}

Result btdevAcquireBleScanEvent(Event* out_event) {
    return btmuAcquireBleScanEvent(out_event);
}

Result btdevGetBleScanParameter(u16 parameter_id, BtdrvBleAdvertisePacketParameter *out) {
    return btmuGetBleScanFilterParameter(parameter_id, out);
}

Result btdevGetBleScanParameter2(u16 parameter_id, BtdrvGattAttributeUuid *out) {
    return btmuGetBleScanFilterParameter2(parameter_id, out);
}

Result btdevStartBleScanGeneral(BtdrvBleAdvertisePacketParameter param) {
    return btmuStartBleScanForGeneral(param);
}

Result btdevStopBleScanGeneral(void) {
    return btmuStopBleScanForGeneral();
}

Result btdevGetBleScanResult(BtdrvBleScanResult *results, u8 count, u8 *total_out) {
    Result rc=0;
    BtdrvBleScanResult tmpresults[2][10]={0};
    u8 tmp_total_out[2]={0};
    u8 tmpi[2]={0};
    u8 targeti=0;

    memset(results, 0, sizeof(BtdrvBleScanResult)*count);

    rc = btmuGetBleScanResultsForGeneral(tmpresults[0], 10, &tmp_total_out[0]);
    if (R_FAILED(rc)) return rc;

    rc = btmuGetBleScanResultsForSmartDevice(tmpresults[1], 10, &tmp_total_out[1]);
    if (R_FAILED(rc)) return rc;

    for (u8 i=0; i<count; i++) {
        if (tmpi[0] < tmp_total_out[0] && tmpi[1] < tmp_total_out[1]) {
            if (tmpresults[0][tmpi[0]].unk_x144 >= tmpresults[1][tmpi[1]].unk_x144) targeti = 0;
            else targeti = 1;
        }
        else {
            if (tmpi[1] >= tmp_total_out[1] && tmpi[0] < tmp_total_out[0]) targeti = 0;
            else if (tmpi[0] >= tmp_total_out[0] && tmpi[1] < tmp_total_out[1]) targeti = 1;
            else break;
        }

        memcpy(results, &tmpresults[targeti][tmpi[targeti]], sizeof(BtdrvBleScanResult));
        results++;
        tmpi[targeti]++;
    }

    if (total_out) *total_out = tmpi[0] + tmpi[1];

    return rc;
}

Result btdevEnableBleAutoConnection(BtdrvBleAdvertisePacketParameter param) {
    return btmuStartBleScanForPaired(param);
}

Result btdevDisableBleAutoConnection(void) {
    return btmuStopBleScanForPaired();
}

Result btdevStartBleScanSmartDevice(const BtdrvGattAttributeUuid *uuid) {
    return btmuStartBleScanForSmartDevice(uuid);
}

Result btdevStopBleScanSmartDevice(void) {
    return btmuStopBleScanForSmartDevice();
}

Result btdevAcquireBleConnectionStateChangedEvent(Event* out_event) {
    return btmuAcquireBleConnectionEvent(out_event);
}

Result btdevConnectToGattServer(BtdrvAddress addr) {
    return btmuBleConnect(addr);
}

Result btdevDisconnectFromGattServer(u32 connection_handle) {
    return btmuBleDisconnect(connection_handle);
}

Result btdevGetBleConnectionInfoList(BtdrvBleConnectionInfo *info, u8 count, u8 *total_out) {
    return btmuBleGetConnectionState(info, count, total_out);
}

Result btdevAcquireBleServiceDiscoveryEvent(Event* out_event) {
    return btmuAcquireBleServiceDiscoveryEvent(out_event);
}

Result btdevGetGattServices(u32 connection_handle, BtdevGattService *services, u8 count, u8 *total_out) {
    Result rc=0;
    u8 tmp_total_out=0;
    BtmGattService tmpservices[100]={0};

    memset(services, 0, sizeof(BtdevGattService)*count);

    rc = btmuGetGattServices(connection_handle, tmpservices, 100, &tmp_total_out);
    if (R_SUCCEEDED(rc)) {
        if (tmp_total_out > count) tmp_total_out = count;
        for (u8 i=0; i<tmp_total_out; i++) {
            btdevGattServiceCreate(&services[i], &tmpservices[i].uuid, tmpservices[i].handle, connection_handle, tmpservices[i].instance_id, tmpservices[i].end_group_handle, tmpservices[i].primary_service);
        }
        if (total_out) *total_out = tmp_total_out;
    }
    return rc;
}

Result btdevGetGattService(u32 connection_handle, const BtdrvGattAttributeUuid *uuid, BtdevGattService *service, bool *flag) {
    Result rc=0;
    bool tmp_flag=0;
    BtmGattService tmpservice={0};

    memset(service, 0, sizeof(BtdevGattService));

    rc = btmuGetGattService(connection_handle, uuid, &tmpservice, &tmp_flag);
    if (R_SUCCEEDED(rc) && tmp_flag) {
        btdevGattServiceCreate(service, uuid, tmpservice.handle, connection_handle, tmpservice.instance_id, tmpservice.end_group_handle, tmpservice.primary_service);
    }
    if (R_SUCCEEDED(rc) && flag) *flag = tmp_flag;
    return rc;
}

Result btdevAcquireBlePairingEvent(Event* out_event) {
    return btmuAcquireBlePairingEvent(out_event);
}

Result btdevPairGattServer(u32 connection_handle, BtdrvBleAdvertisePacketParameter param) {
    return btmuBlePairDevice(connection_handle, param);
}

Result btdevUnpairGattServer(u32 connection_handle, BtdrvBleAdvertisePacketParameter param) {
    return btmuBleUnPairDevice(connection_handle, param);
}

Result btdevUnpairGattServer2(BtdrvAddress addr, BtdrvBleAdvertisePacketParameter param) {
    return btmuBleUnPairDevice2(addr, param);
}

Result btdevGetPairedGattServerAddress(BtdrvBleAdvertisePacketParameter param, BtdrvAddress *addrs, u8 count, u8 *total_out) {
    return btmuBleGetPairedDevices(param, addrs, count, total_out);
}

Result btdevAcquireBleMtuConfigEvent(Event* out_event) {
    return btmuAcquireBleMtuConfigEvent(out_event);
}

Result btdevConfigureBleMtu(u32 connection_handle, u16 mtu) {
    if (mtu < 0x18 || mtu > 0x200) return MAKERESULT(Module_Libnx, LibnxError_BadInput); // sdknso would Abort here, sdknso impls the same thing in the btmu func as well (but we don't).
    return btmuConfigureBleMtu(connection_handle, mtu);
}

Result btdevGetBleMtu(u32 connection_handle, u16 *out) {
    return btmuGetBleMtu(connection_handle, out);
}

Result btdevAcquireBleGattOperationEvent(Event* out_event) {
    return btRegisterBleEvent(out_event);
}

Result btdevRegisterGattOperationNotification(const BtdrvGattAttributeUuid *uuid) {
    BtmBleDataPath path = {.unk_x0 = 0, .uuid = *uuid};
    return btmuRegisterBleGattDataPath(&path);
}

Result btdevUnregisterGattOperationNotification(const BtdrvGattAttributeUuid *uuid) {
    BtmBleDataPath path = {.unk_x0 = 0, .uuid = *uuid};
    return btmuUnregisterBleGattDataPath(&path);
}

Result btdevGetGattOperationResult(BtdrvBleClientGattOperationInfo *out) {
    Result rc=0;
    u32 type=0;
    u8 tmpval=0;
    BtdrvLeEventInfo eventinfo={0};

    if (out==NULL) return MAKERESULT(Module_Libnx, LibnxError_BadInput);
    memset(out, 0, sizeof(*out));

    rc = btGetLeEventInfo(&eventinfo, sizeof(eventinfo), &type);
    if (R_SUCCEEDED(rc) && type!=8) rc = MAKERESULT(Module_Libnx, LibnxError_ShouldNotHappen);
    if (R_SUCCEEDED(rc)) {
        switch(eventinfo.unk_x0) {
            case 0:
                tmpval=0;
            break;

            case 4:
            case 16:
            case 21:
            case 50:
                tmpval=3;
            break;

            case 5:
                tmpval=4;
            break;

            case 6:
                tmpval=5;
            break;

            case 7:
                tmpval=6;
            break;

            case 9:
                tmpval=7;
            break;

            case 13:
                tmpval=8;
            break;

            case 25:
            case 52:
                tmpval=2;
            break;

            case 35:
                tmpval=10;
            break;

            case 55:
                tmpval=1;
            break;

            default:
                tmpval=9;
            break;
        }

        out->unk_x0 = tmpval;
        out->unk_x4 = eventinfo.unk_x4;
        out->unk_x8 = eventinfo.unk_x8;
        out->uuid0 = eventinfo.uuid0;
        out->uuid1 = eventinfo.uuid1;
        out->uuid2 = eventinfo.uuid2;

        out->size = eventinfo.size;
        if (out->size > sizeof(out->data)) out->size = sizeof(out->data);
        memcpy(out->data, eventinfo.data, out->size);
    }

    return rc;
}

Result btdevReadGattCharacteristic(BtdevGattCharacteristic *c) {
    Result rc=0;
    BtdevGattService tmpservice={0};
    BtdrvGattId gattid0={0};
    BtdrvGattId gattid1={0};

    if ((btdevGattCharacteristicGetProperties(c) & BIT(1)) == 0)
        rc = MAKERESULT(113, 523);

    if (R_SUCCEEDED(rc)) rc = btdevGattCharacteristicGetService(c, &tmpservice);

    if (R_SUCCEEDED(rc)) {
        gattid0.instance_id = btdevGattServiceGetInstanceId(&tmpservice);
        btdevGattAttributeGetUuid(&tmpservice.attr, &gattid0.uuid);
        gattid1.instance_id = btdevGattCharacteristicGetInstanceId(c);
        btdevGattAttributeGetUuid(&c->attr, &gattid1.uuid);
        rc = btLeClientReadCharacteristic(btdevGattAttributeGetConnectionHandle(&c->attr), btdevGattServiceIsPrimaryService(&tmpservice), &gattid0, &gattid1, 0);
    }

    return rc;
}

Result btdevWriteGattCharacteristic(BtdevGattCharacteristic *c) {
    Result rc=0;
    BtdevGattService tmpservice={0};
    BtdrvGattId gattid0={0};
    BtdrvGattId gattid1={0};
    u8 prop = btdevGattCharacteristicGetProperties(c);
    u64 value_size=0;
    u8 value[0x200]={0};

    if ((prop & (BIT(2) | BIT(3))) == 0)
        rc = MAKERESULT(113, 523);

    if (R_SUCCEEDED(rc)) value_size = btdevGattCharacteristicGetValue(c, value, sizeof(value));

    if (R_SUCCEEDED(rc)) rc = btdevGattCharacteristicGetService(c, &tmpservice);

    if (R_SUCCEEDED(rc)) {
        gattid0.instance_id = btdevGattServiceGetInstanceId(&tmpservice);
        btdevGattAttributeGetUuid(&tmpservice.attr, &gattid0.uuid);
        gattid1.instance_id = btdevGattCharacteristicGetInstanceId(c);
        btdevGattAttributeGetUuid(&c->attr, &gattid1.uuid);
        rc = btLeClientWriteCharacteristic(btdevGattAttributeGetConnectionHandle(&c->attr), btdevGattServiceIsPrimaryService(&tmpservice), &gattid0, &gattid1, value, value_size, 0, (prop & BIT(3)) != 0);
    }

    return rc;
}

Result btdevEnableGattCharacteristicNotification(BtdevGattCharacteristic *c, bool flag) {
    Result rc=0;
    BtdevGattService tmpservice={0};
    BtdrvGattId gattid0={0};
    BtdrvGattId gattid1={0};
    u32 connection_handle = btdevGattAttributeGetConnectionHandle(&c->attr);

    if ((btdevGattCharacteristicGetProperties(c) & (BIT(4) | BIT(5))) == 0)
        rc = MAKERESULT(113, 523);

    if (R_SUCCEEDED(rc)) rc = btdevGattCharacteristicGetService(c, &tmpservice);

    if (R_SUCCEEDED(rc)) {
        gattid0.instance_id = btdevGattServiceGetInstanceId(&tmpservice);
        btdevGattAttributeGetUuid(&tmpservice.attr, &gattid0.uuid);
        gattid1.instance_id = btdevGattCharacteristicGetInstanceId(c);
        btdevGattAttributeGetUuid(&c->attr, &gattid1.uuid);
        bool primary_service = btdevGattServiceIsPrimaryService(&tmpservice);
        if (flag)
            rc = btLeClientRegisterNotification(connection_handle, primary_service, &gattid0, &gattid1);
        else
            rc = btLeClientDeregisterNotification(connection_handle, primary_service, &gattid0, &gattid1);
    }

    return rc;
}

Result btdevReadGattDescriptor(BtdevGattDescriptor *d) {
    Result rc=0;
    BtdevGattService tmpservice={0};
    BtdevGattCharacteristic tmpcharacteristic={0};
    BtdrvGattId gattid0={0};
    BtdrvGattId gattid1={0};
    BtdrvGattId gattid2={0}; // instance_id is left at 0.

    rc = btdevGattDescriptorGetService(d, &tmpservice);
    if (R_SUCCEEDED(rc)) rc = btdevGattDescriptorGetCharacteristic(d, &tmpcharacteristic);

    if (R_SUCCEEDED(rc)) {
        gattid0.instance_id = btdevGattServiceGetInstanceId(&tmpservice);
        btdevGattAttributeGetUuid(&tmpservice.attr, &gattid0.uuid);
        gattid1.instance_id = btdevGattCharacteristicGetInstanceId(&tmpcharacteristic);
        btdevGattAttributeGetUuid(&tmpcharacteristic.attr, &gattid1.uuid);
        btdevGattAttributeGetUuid(&d->attr, &gattid2.uuid);
        rc = btLeClientReadDescriptor(btdevGattAttributeGetConnectionHandle(&d->attr), btdevGattServiceIsPrimaryService(&tmpservice), &gattid0, &gattid1, &gattid2, 0);
    }

    return rc;
}

Result btdevWriteGattDescriptor(BtdevGattDescriptor *d) {
    Result rc=0;
    BtdevGattService tmpservice={0};
    BtdevGattCharacteristic tmpcharacteristic={0};
    BtdrvGattId gattid0={0};
    BtdrvGattId gattid1={0};
    BtdrvGattId gattid2={0}; // instance_id is left at 0.
    u64 value_size=0;
    u8 value[0x200]={0};

    value_size = btdevGattDescriptorGetValue(d, value, sizeof(value));

    rc = btdevGattDescriptorGetService(d, &tmpservice);
    if (R_SUCCEEDED(rc)) rc = btdevGattDescriptorGetCharacteristic(d, &tmpcharacteristic);

    if (R_SUCCEEDED(rc)) {
        gattid0.instance_id = btdevGattServiceGetInstanceId(&tmpservice);
        btdevGattAttributeGetUuid(&tmpservice.attr, &gattid0.uuid);
        gattid1.instance_id = btdevGattCharacteristicGetInstanceId(&tmpcharacteristic);
        btdevGattAttributeGetUuid(&tmpcharacteristic.attr, &gattid1.uuid);
        btdevGattAttributeGetUuid(&d->attr, &gattid2.uuid);
        rc = btLeClientWriteDescriptor(btdevGattAttributeGetConnectionHandle(&d->attr), btdevGattServiceIsPrimaryService(&tmpservice), &gattid0, &gattid1, &gattid2, value, value_size, 0);
    }

    return rc;
}

// GattAttribute

void btdevGattAttributeCreate(BtdevGattAttribute *a, const BtdrvGattAttributeUuid *uuid, u16 handle, u32 connection_handle) {
    memset(a, 0, sizeof(*a));
    a->type = 0xff;
    a->uuid = *uuid;
    a->handle = handle;
    a->connection_handle = connection_handle;
}

// GattService

void btdevGattServiceCreate(BtdevGattService *s, const BtdrvGattAttributeUuid *uuid, u16 handle, u32 connection_handle, u16 instance_id, u16 end_group_handle, bool primary_service) {
    memset(s, 0, sizeof(*s));
    btdevGattAttributeCreate(&s->attr, uuid, handle, connection_handle);
    s->attr.type = 0x3;
    s->instance_id = instance_id;
    s->end_group_handle = end_group_handle;
    s->primary_service = primary_service;
}

Result btdevGattServiceGetIncludedServices(BtdevGattService *s, BtdevGattService *services, u8 count, u8 *total_out) {
    Result rc=0;
    u8 tmp_total_out=0;
    BtmGattService tmpservices[100]={0};

    memset(services, 0, sizeof(BtdevGattService)*count);

    rc = btmuGetGattIncludedServices(s->attr.connection_handle, s->attr.handle, tmpservices, 100, &tmp_total_out);
    if (R_SUCCEEDED(rc)) {
        if (tmp_total_out > count) tmp_total_out = count;
        for (u8 i=0; i<tmp_total_out; i++) {
            btdevGattServiceCreate(&services[i], &tmpservices[i].uuid, tmpservices[i].handle, s->attr.connection_handle, tmpservices[i].instance_id, tmpservices[i].end_group_handle, tmpservices[i].primary_service);
        }
        if (total_out) *total_out = tmp_total_out;
    }
    return rc;
}

Result btdevGattServiceGetCharacteristics(BtdevGattService *s, BtdevGattCharacteristic *characteristics, u8 count, u8 *total_out) {
    Result rc=0;
    BtmGattCharacteristic tmpcharacteristics[100]={0};
    u8 tmp_total_out=0;

    if (R_SUCCEEDED(rc)) rc = btmuGetGattCharacteristics(s->attr.connection_handle, s->attr.handle, tmpcharacteristics, 100, &tmp_total_out);
    if (R_SUCCEEDED(rc)) {
        if (tmp_total_out > count) tmp_total_out = count;
        for (u8 i=0; i<tmp_total_out; i++) {
            btdevGattCharacteristicCreate(&characteristics[i], &tmpcharacteristics[i].uuid, tmpcharacteristics[i].handle, s->attr.connection_handle, tmpcharacteristics[i].instance_id, tmpcharacteristics[i].properties);
        }
        if (total_out) *total_out = tmp_total_out;
    }
    return rc;
}

Result btdevGattServiceGetCharacteristic(BtdevGattService *s, const BtdrvGattAttributeUuid *uuid, BtdevGattCharacteristic *characteristic, bool *flag) {
    Result rc=0;
    BtmGattCharacteristic tmpcharacteristics[100]={0};
    u8 tmp_total_out=0;
    bool tmp_flag=false;

    if (R_SUCCEEDED(rc)) rc = btmuGetGattCharacteristics(s->attr.connection_handle, s->attr.handle, tmpcharacteristics, 100, &tmp_total_out);
    if (R_SUCCEEDED(rc)) {
        if (tmp_total_out > 100) tmp_total_out = 100;
        for (u8 i=0; i<tmp_total_out; i++) {
            if (btdevGattAttributeUuidIsSame(&tmpcharacteristics[i].uuid, uuid)) {
                btdevGattCharacteristicCreate(characteristic, &tmpcharacteristics[i].uuid, tmpcharacteristics[i].handle, s->attr.connection_handle, tmpcharacteristics[i].instance_id, tmpcharacteristics[i].properties);
                tmp_flag=true;
                break;
            }
        }
        if (flag) *flag = tmp_flag;
    }
    return rc;
}

// GattCharacteristic

void btdevGattCharacteristicCreate(BtdevGattCharacteristic *c, const BtdrvGattAttributeUuid *uuid, u16 handle, u32 connection_handle, u16 instance_id, u8 properties) {
    memset(c, 0, sizeof(*c));
    btdevGattAttributeCreate(&c->attr, uuid, handle, connection_handle);
    c->attr.type = 0x1;
    c->instance_id = instance_id;
    c->properties = properties;
}

Result btdevGattCharacteristicGetService(BtdevGattCharacteristic *c, BtdevGattService *service) {
    Result rc=0;
    BtmGattService tmpservice={0};
    bool flag=0;

    rc = btmuGetBelongingGattService(c->attr.connection_handle, c->attr.handle, &tmpservice, &flag);
    if (R_SUCCEEDED(rc) && !flag) rc = MAKERESULT(113, 512);
    if (R_SUCCEEDED(rc)) btdevGattServiceCreate(service, &tmpservice.uuid, tmpservice.handle, c->attr.connection_handle, tmpservice.instance_id, tmpservice.end_group_handle, tmpservice.primary_service);
    return rc;
}

Result btdevGattCharacteristicGetDescriptors(BtdevGattCharacteristic *c, BtdevGattDescriptor *descriptors, u8 count, u8 *total_out) {
    Result rc=0;
    u8 tmp_total_out=0;
    BtmGattDescriptor tmpdescriptors[100]={0};

    memset(descriptors, 0, sizeof(BtdevGattDescriptor)*count);

    rc = btmuGetGattDescriptors(c->attr.connection_handle, c->attr.handle, tmpdescriptors, 100, &tmp_total_out);
    if (R_SUCCEEDED(rc)) {
        if (tmp_total_out > count) tmp_total_out = count;
        for (u8 i=0; i<tmp_total_out; i++) {
            btdevGattDescriptorCreate(&descriptors[i], &tmpdescriptors[i].uuid, tmpdescriptors[i].handle, c->attr.connection_handle);
        }
        if (total_out) *total_out = tmp_total_out;
    }
    return rc;
}

Result btdevGattCharacteristicGetDescriptor(BtdevGattCharacteristic *c, const BtdrvGattAttributeUuid *uuid, BtdevGattDescriptor *descriptor, bool *flag) {
    Result rc=0;
    u8 tmp_total_out=0;
    bool tmp_flag=0;
    BtmGattDescriptor tmpdescriptors[100]={0};

    memset(descriptor, 0, sizeof(BtdevGattDescriptor));

    rc = btmuGetGattDescriptors(c->attr.connection_handle, c->attr.handle, tmpdescriptors, 100, &tmp_total_out);
    if (R_SUCCEEDED(rc)) {
        if (tmp_total_out > 100) tmp_total_out = 100;
        for (u8 i=0; i<tmp_total_out; i++) {
            if (btdevGattAttributeUuidIsSame(&tmpdescriptors[i].uuid, uuid)) {
                btdevGattDescriptorCreate(descriptor, &tmpdescriptors[i].uuid, tmpdescriptors[i].handle, c->attr.connection_handle);
                tmp_flag = true;
                break;
            }
        }
        if (flag) *flag = tmp_flag;
    }
    return rc;
}

void btdevGattCharacteristicSetValue(BtdevGattCharacteristic *c, const void* buffer, size_t size) {
    if (buffer==NULL || size > sizeof(c->value)) return;
    c->value_size = size;
    memcpy(c->value, buffer, size);
}

u64 btdevGattCharacteristicGetValue(BtdevGattCharacteristic *c, void* buffer, size_t size) {
    if (buffer==NULL) return 0;
    u64 out_size = c->value_size;
    if (size < out_size) out_size = size;
    if (out_size > sizeof(c->value)) out_size = sizeof(c->value);
    memcpy(buffer, c->value, out_size);
    return out_size;
}

// GattDescriptor

void btdevGattDescriptorCreate(BtdevGattDescriptor *d, const BtdrvGattAttributeUuid *uuid, u16 handle, u32 connection_handle) {
    memset(d, 0, sizeof(*d));
    btdevGattAttributeCreate(&d->attr, uuid, handle, connection_handle);
    d->attr.type = 0x2;
}

Result btdevGattDescriptorGetService(BtdevGattDescriptor *d, BtdevGattService *service) {
    Result rc=0;
    BtmGattService tmpservice={0};
    bool flag=0;

    rc = btmuGetBelongingGattService(d->attr.connection_handle, d->attr.handle, &tmpservice, &flag);
    if (R_SUCCEEDED(rc) && !flag) rc = MAKERESULT(113, 512);
    if (R_SUCCEEDED(rc)) btdevGattServiceCreate(service, &tmpservice.uuid, tmpservice.handle, d->attr.connection_handle, tmpservice.instance_id, tmpservice.end_group_handle, tmpservice.primary_service);
    return rc;
}

Result btdevGattDescriptorGetCharacteristic(BtdevGattDescriptor *d, BtdevGattCharacteristic *characteristic) {
    Result rc=0;
    BtdevGattService tmpservice={0};
    BtmGattCharacteristic tmpcharacteristics[100]={0};
    u8 tmp_total_out=0;

    rc = btdevGattDescriptorGetService(d, &tmpservice);

    if (R_SUCCEEDED(rc)) rc = btmuGetGattCharacteristics(d->attr.connection_handle, btdevGattAttributeGetHandle(&tmpservice.attr), tmpcharacteristics, 100, &tmp_total_out);
    if (R_SUCCEEDED(rc)) {
        if (tmp_total_out > 100) tmp_total_out = 100;
        for (u8 i=0; i<tmp_total_out; i++) {
            if (tmpcharacteristics[i].handle < d->attr.handle && (tmp_total_out-1 == i || d->attr.handle < tmpcharacteristics[i+1].handle)) {
                btdevGattCharacteristicCreate(characteristic, &tmpcharacteristics[i].uuid, tmpcharacteristics[i].handle, d->attr.connection_handle, tmpcharacteristics[i].instance_id, tmpcharacteristics[i].properties);
                return 0;
            }
        }
    }
    if (R_SUCCEEDED(rc)) rc = MAKERESULT(113, 512);
    return rc;
}

void btdevGattDescriptorSetValue(BtdevGattDescriptor *d, const void* buffer, size_t size) {
    if (buffer==NULL || size > sizeof(d->value)) return;
    d->value_size = size;
    memcpy(d->value, buffer, size);
}

u64 btdevGattDescriptorGetValue(BtdevGattDescriptor *d, void* buffer, size_t size) {
    if (buffer==NULL) return 0;
    u64 out_size = d->value_size;
    if (size < out_size) out_size = size;
    if (out_size > sizeof(d->value)) out_size = sizeof(d->value);
    memcpy(buffer, d->value, out_size);
    return out_size;
}

