from typing import Callable

import requests
import socket
from PIL import Image

from datetime import datetime, timedelta

from artnet import ArtnetProvider

from util import grouper, flatmap, bilinear, BufferedResource, RepeatTimer, RegularClock


def pixel_at(image: Image, x: float, y: float):
    r = bilinear(image, x * (image.width - 1), y * (image.height - 1))
    return r


def run(
    ip: str,
    endpoint: str,
    image_provider: Callable[[], Image.Image],
    frames_per_second: int = 30
):
    print("Getting Server Info")

    server_info = requests.get(f"http://{ip}/i").json()
    # print(server_info)

    endpoint_info = server_info[endpoint]
    net = int(endpoint_info["net"])

    port = 6454  # default artnet port

    if endpoint == "concentric":
        pixels = list(grouper(2, endpoint_info["pixels"]))
        print(f"Sampled Pixels: {pixels}")
    elif endpoint == "cartesian":
        resolution = (endpoint_info["width"], endpoint_info["height"])
        print(f"Screen Size: {resolution[0]}x{resolution[1]}")

    # ------------------------------------------------------
    # --- Frame Conversion
    # ------------------------------------------------------

    def grab_frame():
        img = image_provider()

        if endpoint == "concentric":
            return bytes(flatmap(
                lambda t: pixel_at(img, *t),
                pixels
            ))
        else:
            img = img.resize(resolution)
            # img.save("backup.png")
            return img.tobytes("raw")

    data_resource = BufferedResource(max_buffer_size=2)
    capture_frame_thread = RepeatTimer(
        interval=0,
        function=lambda: data_resource.push(grab_frame())
    )
    capture_frame_thread.start()

    # ------------------------------------------------------
    # --- Packet Assembly
    # ------------------------------------------------------

    artnet_provider = ArtnetProvider(
        universe=0,
        subnet=0,
        net=net
    )

    def grab_artnet_packets():
        return list(map(bytes, artnet_provider(data_resource.pop())))

    packets_resource = BufferedResource(max_buffer_size=2)
    capture_packets_thread = RepeatTimer(
        interval=0,
        function=lambda: packets_resource.push(grab_artnet_packets())
    )
    capture_packets_thread.start()

    # ------------------------------------------------------
    # --- Loop
    # ------------------------------------------------------

    sock = socket.socket(
        socket.AF_INET,  # Internet
        socket.SOCK_DGRAM  # UDP
    )

    time_per_frame = timedelta(seconds=1.0 / frames_per_second)

    print(f"Sending Art-Net Data to: {ip}:{port} — {artnet_provider.net}|{artnet_provider.subnet}|{artnet_provider.universe}!")
    start = datetime.now()
    sequence_start = start

    clock = RegularClock("FPS")
    frame_start = clock.mark()

    while True:
        packets = packets_resource.pop()
        for packet in packets:
            sock.sendto(
                packet,
                (ip, port)
            )

        if artnet_provider.sequence == 0:
            print(
                f"Sequence Pushed! kB: {sum(map(len, packets)) / 3}"
                f", Packets p.f.: {len(packets)}"
                f", FPS: {255.0 / (frame_start - sequence_start).total_seconds()}"
            )
            sequence_start = frame_start

        # For plotting coords
        # log = requests.get(f"http://{ip}/log")
        # img2 = img.copy()
        # for idx, x, y in grouper(3, log.text.split("\n")):
        #     if x and y:
        #         print(idx, x, y)
        #         img2.putpixel((int(x), int(y)), (int(idx) * 14, 0, 255))
        # img2.save("rs.png")

        frame_start = clock.elapse(time_per_frame)
