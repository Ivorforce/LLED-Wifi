//
// Created by Lukas Tenbrink on 17.02.20.
//

#include "CharRoller.h"
#include "../../../../.platformio/packages/framework-arduinoespressif32/cores/esp32/Arduino.h"
#include "../../../../.platformio/packages/toolchain-xtensa32/xtensa-esp32-elf/include/string.h"

CharRoller::CharRoller(const int c) : count(c), head(0) {
    data = new char[count];
    clear();
}

void CharRoller::push(char c) {
    data[head] = c;
    head = (head + 1) % count;
}

void CharRoller::push(char *d) {
    push(data, String(data).length());
}

void CharRoller::push(String data) {
    push(data.begin(), data.length());
}

void CharRoller::push(char *d, unsigned int length) {
    if (length > count) {
        memcpy(data, d + (length - count), count);
        head = 0;
        return;
    }

    int fromHead = _min(length, count - head);
    memcpy(data + head, d, fromHead);
    head += fromHead;

    if (length - fromHead > 0) {
        // Roll Over
        memcpy(data, d + fromHead, length - fromHead);
        head = length - fromHead;
    }
}

String CharRoller::toString() {
    char *left = new char[count - head + 1];
    memcpy(left, data + head, count - head);
    left[count - head] = '\0';

    char *right = new char[head + 1];
    memcpy(right, data, head);
    right[head - 1] = '\0';

    return String(left) + String(right);
}

void CharRoller::clear() {
    head = 0;
    memset(data, '\0', count);
}
