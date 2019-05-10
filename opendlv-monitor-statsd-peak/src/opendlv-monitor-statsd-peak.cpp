/*
 * Copyright (C) 2019  Christian Berger
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "cluon-complete.hpp"
#include "opendlv-standard-message-set.hpp"
#include "peak-can.hpp"

#include <cstdint>
#include <iostream>
#include <memory>
#include <mutex>

int32_t main(int32_t argc, char **argv) {
    int32_t retCode{1};
    auto commandlineArguments = cluon::getCommandlineArguments(argc, argv);
    if ( (0 == commandlineArguments.count("cid")) ) {
        std::cerr << argv[0] << " joins a running OD4 session to relay selected messages from PEAK GPS to statsd." << std::endl;
        std::cerr << "Usage:   " << argv[0] << " --cid=<OD4 session> [--verbose]" << std::endl;
        std::cerr << "         --cid:    CID of the OD4Session to receive messages" << std::endl;
        std::cerr << "Example: " << argv[0] << " --cid=111 --verbose" << std::endl;
    }
    else {
        const bool VERBOSE{commandlineArguments.count("verbose") != 0};

        // Interface to a running OpenDaVINCI session; here, you can receive messages.
        cluon::OD4Session od4{static_cast<uint16_t>(std::stoi(commandlineArguments["cid"]))};

        std::mutex dataMutex;

        opendlv::proxy::GroundSpeedReading speed;
        opendlv::device::gps::peak::GPSStatus status;

        auto onGroundSpeed = [&dataMutex, &speed](cluon::data::Envelope &&env){
            std::lock_guard<std::mutex> lck(dataMutex);
            speed = cluon::extractMessage<opendlv::proxy::GroundSpeedReading>(std::move(env));
        };
        auto onGPSStatus = [&dataMutex, &status](cluon::data::Envelope &&env){
            std::lock_guard<std::mutex> lck(dataMutex);
            status = cluon::extractMessage<opendlv::device::gps::peak::GPSStatus>(std::move(env));
        };

        od4.dataTrigger(opendlv::proxy::GroundSpeedReading::ID(), onGroundSpeed);
        od4.dataTrigger(opendlv::device::gps::peak::GPSStatus::ID(), onGPSStatus);

        // Time-triggered sender.
        const float FIFTEEN_SECONDS{15.0f};
        od4.timeTrigger(1.0f/FIFTEEN_SECONDS, [&od4, &dataMutex, &status, &speed, VERBOSE](){
            std::lock_guard<std::mutex> lck(dataMutex);

            cluon::UDPSender sender("127.0.0.1", 8125);
            {
                std::stringstream sstr;
                sstr << "PEAK-GPS.numberOfSatellites:" << status.numberOfSatellites() << "|g";
                std::string s = sstr.str();
                sender.send(std::move(s));
            }
            {
                std::stringstream sstr;
                sstr.precision(1);
                sstr << "PEAK-GPS.speed:" << speed.groundSpeed() << "|g";
                std::string s = sstr.str();
                sender.send(std::move(s));
            }

            return od4.isRunning();
        });

        retCode = 0;
    }
    return retCode;
}

