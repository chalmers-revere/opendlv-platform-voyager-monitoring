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

#include <cstdint>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <thread>

int32_t main(int32_t argc, char **argv) {
    int32_t retCode{1};
    auto commandlineArguments = cluon::getCommandlineArguments(argc, argv);
    if ( (0 == commandlineArguments.count("cid")) ) {
        std::cerr << argv[0] << " joins a running OD4 session to extract the current latitude/longitude information from PEAK GPS." << std::endl;
        std::cerr << "Usage:   " << argv[0] << " --cid=<OD4 session>" << std::endl;
        std::cerr << "         --cid:    CID of the OD4Session to receive messages" << std::endl;
        std::cerr << "Example: " << argv[0] << " --cid=111" << std::endl;
    }
    else {
        // Interface to a running OpenDaVINCI session; here, you can receive messages.
        cluon::OD4Session od4{static_cast<uint16_t>(std::stoi(commandlineArguments["cid"]))};

        std::atomic<bool> received{false};
        auto onGPSPosition = [&received](cluon::data::Envelope &&env){
            if (!received) {
                received = true;
                opendlv::proxy::GeodeticWgs84Reading pos = cluon::extractMessage<opendlv::proxy::GeodeticWgs84Reading>(std::move(env));
                std::cout << "latitude=" << std::setprecision(10) << pos.latitude() << "," << "longitude=" << pos.longitude();

                cluon::TerminateHandler::instance().isTerminated.store(true);
            }
        };

        od4.dataTrigger(opendlv::proxy::GeodeticWgs84Reading::ID(), onGPSPosition);

        int counter{10};
        while (!cluon::TerminateHandler::instance().isTerminated.load() && counter > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            counter--;
        }

        retCode = 0;
    }
    return retCode;
}

