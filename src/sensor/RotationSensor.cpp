//
// Created by Lukas Tenbrink on 20.01.20.
//

#include <cmath>
#include <utility>
#include <util/Logger.h>
#include <util/Interrupts.h>
#include "RotationSensor.h"

#include "Setup.h"

RotationSensor::RotationSensor(std::vector<SensorSwitch*> switches, int historySize, int minCheckpointPasses, Extrapolator *extrapolator) :
    switches(std::move(switches)),
    checkpointTimestamps(new IntRoller(historySize)),
    checkpointIndices(new IntRoller(historySize)),
    minCheckpointPasses(minCheckpointPasses),
    extrapolator(extrapolator)  {

#if ROTATION_SENSOR_TYPE == ROTATION_SENSOR_TYPE_INTERRUPT
    attachSwitchInterrupts();
#endif
}

void RotationSensor::attachSwitchInterrupts() {
    for (int i = 0; i < switches.size(); i++) {
        attachInterruptFunction(switches[i]->pin, [this, i]() {
            registerCheckpoint(micros(), i);
        }, FALLING, INPUT_PULLUP);
    }
}

void RotationSensor::update(unsigned long time) {
    if (isReliable && time - checkpointTimestamps->last() > 2000 * 1000) {
        // We are paused, clear history
        checkpointTimestamps->fill(0);
        checkpointIndices->fill(-1);
        isReliable = false;
    }

#if ROTATION_SENSOR_TYPE == ROTATION_SENSOR_TYPE_HALL_SYNC
    for (int i = 0; i < switches.size(); ++i) {
        SensorSwitch *sensorSwitch = switches[i];

        // Test the switch
        if (sensorSwitch->test() && sensorSwitch->isReliable) {
            if (!sensorSwitch->isOn()) {
                registerCheckpoint(time, i);
            }
        }

        isReliable &= sensorSwitch->isReliable;
    }
#endif
}

void RotationSensor::registerCheckpoint(unsigned long time, int checkpoint) {
    unsigned int historySize = checkpointIndices->count;
    unsigned int checkpointCount = switches.size();

    checkpointIndices->append(checkpoint);
    checkpointTimestamps->append(time);

    int n = (int) historySize - checkpointIndices->countOccurrences(-1);

    if (n < minCheckpointPasses) {
        // Too little data to make meaningful extrapolation
        isReliable = false;
        return;
    }

    // Raw Data Collection
    std::vector<double> x{};
    x.reserve(n);
    std::vector<int> estimatedY{};
    estimatedY.reserve(n);

    for (int j = 0; j < historySize; ++j) {
        int checkpointIndex = (*checkpointIndices)[j];

        if (checkpointIndex < 0)
            continue; // Not set yet

        x.push_back((*checkpointTimestamps)[j]);
        estimatedY.push_back(checkpointIndex);
    }

    double estimatedCheckpointDiff = INT_MAX;
    for (int j = 1; j < n; ++j) {
        int xDiff = (*checkpointTimestamps)[j] - (*checkpointTimestamps)[j - 1];
        if (xDiff < estimatedCheckpointDiff)
            estimatedCheckpointDiff = xDiff;
    }

    // Try to estimate if we missed any checkpoints
    std::vector<double> y(n);

    if (estimatedCheckpointDiff <= 0) {
        // Not sure what happened... but don't take the chance.
        Logger::println("xDiffMean = 0; unable to sync rotation...");
        isReliable = false;
        return;
    }

    // Go back in reverse, setting reached checkpoint as baseline Y
    y[n - 1] = estimatedY[n - 1];
    for (int j = n - 2; j >= 0; --j) {
        unsigned int expectedSteps = (estimatedY[j + 1] - estimatedY[j] + checkpointCount) % checkpointCount;
        double estimatedSteps = (x[j + 1] - x[j]) / estimatedCheckpointDiff;

        // Accept +- multiples of checkpointCount
        y[j] = y[j + 1] - round((estimatedSteps - expectedSteps) / checkpointCount) * checkpointCount + expectedSteps;
    }

    extrapolator->adjust(x, y);
    double rotationsPerSecond = this->rotationsPerSecond();

    // Speed is sensible?
    isReliable = rotationsPerSecond < 100 && rotationsPerSecond > 1;
}

float RotationSensor::estimatedRotation(unsigned long time) {
    if (!isReliable)
        return NAN;

    float rawRotation = extrapolator->extrapolate(time);

    if (std::isnan(rawRotation) || rawRotation > 3.5) {
        // Missed 3 checkpoints, this is not secure at all
        return NAN;
    }

    return std::fmod(rawRotation / switches.size(), 1.0f);
}

int RotationSensor::rotationsPerSecond() {
    return (int) (extrapolator->slope() * 1000 * 1000);
}
