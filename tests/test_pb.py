# -*- coding: utf-8 -*-
import os
import unittest
import struct

import pb
import deviceapps_pb2

MAGIC = 0xFFFFFFFF
DEVICE_APPS_TYPE = 1
TEST_FILE = "test.pb.gz"


class TestPB(unittest.TestCase):
    deviceapps = [
        {"device": {"type": "idfa", "id": "e7e1a50c0ec2747ca56cd9e1558c0d7c"},
         "lat": 67.7835424444, "lon": -22.8044005471, "apps": [1, 2, 3, 4]},
        {"device": {"type": "gaid", "id": "e7e1a50c0ec2747ca56cd9e1558c0d7d"}, "lat": 42, "lon": -42, "apps": [1, 2]},
        {"device": {"type": "gaid", "id": "e7e1a50c0ec2747ca56cd9e1558c0d7d"}, "lat": 42, "lon": -42, "apps": []},
        {"device": {"type": "gaid", "id": "e7e1a50c0ec2747ca56cd9e1558c0d7d"}, "apps": [1]},
    ]

    def tearDown(self):
        pass
        os.remove(TEST_FILE)

    def test_write(self):
        i = 0  # Для итерации по нашему тестовому массиву
        magic_str = struct.pack('I', MAGIC)
        bytes_written = pb.deviceapps_xwrite_pb(iter(self.deviceapps), TEST_FILE)
        self.assertTrue(bytes_written > 0)

        unpacked = deviceapps_pb2.DeviceApps()
        with open(TEST_FILE, 'r') as f:
            while True:
                buf = f.read(4)  # Ищем магическую последовательность
                if not buf:
                    break
                if buf == magic_str:
                    pb_type, pb_len = struct.unpack('HH', f.read(4))
                    message = f.read(pb_len)
                    unpacked.ParseFromString(message)

                device = self.deviceapps[i].get("device", None)
                if device:
                    device_type = self.deviceapps[i]["device"].get("type", None)
                    device_id = self.deviceapps[i]["device"].get("id", None)
                    self.assertEqual(unpacked.device.type, device_type)
                    self.assertEqual(unpacked.device.id, device_id)

                lat = self.deviceapps[i].get("lat", None)
                lon = self.deviceapps[i].get("lon", None)

                if lat:
                    self.assertEqual(unpacked.lat, lat)
                if lon:
                    self.assertEqual(unpacked.lon, lon)
                apps = self.deviceapps[i].get("apps", None)
                if apps:
                    self.assertListEqual(list(unpacked.apps), apps)

                i += 1

    @unittest.skip("Optional problem")
    def test_read(self):
        pb.deviceapps_xwrite_pb(self.deviceapps, TEST_FILE)
        for i, d in enumerate(pb.deviceapps_xread_pb(TEST_FILE)):
            self.assertEqual(d, self.deviceapps[i])

