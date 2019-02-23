/*

This file is part of VROOM.

Copyright (c) 2015-2018, Julien Coupey.
All rights reserved (see LICENSE).

*/

#include "utils/input_parser.h"

#include <array>
#include <vector>

#include "osrm/exception.hpp"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "routing/libosrm_wrapper.h"
#include "routing/orshttp_wrapper.h"
#include "routing/routed_wrapper.h"
#include "structures/cl_args.h"
#include "structures/generic/matrix.h"
#include "structures/typedefs.h"
#include "structures/vroom/amount.h"
#include "structures/vroom/job.h"
#include "structures/vroom/time_window.h"
#include "structures/vroom/vehicle.h"
#include "utils/exception.h"

namespace vroom
{
    namespace io
    {
        // Helper to get optional array of coordinates.
        inline Coordinates parse_coordinates(const rapidjson::Value& object, const char* key)
        {
            if (!object[key].IsArray() or (object[key].Size() < 2) or !object[key][0].IsNumber() or
                !object[key][1].IsNumber()) {
                throw Exception(ERROR::INPUT, "Invalid " + std::string(key) + " array.");
            }
            return {{object[key][0].GetDouble(), object[key][1].GetDouble()}};
        }

        inline std::string get_string(const rapidjson::Value& object, const char* key)
        {
            assert(object.HasMember(key));
            return object[key].GetString();
        }

        inline Amount get_amount(const rapidjson::Value& object, const char* key)
        {
            // Default to empty amount.
            Amount amount(0);

            if (object.HasMember(key)) {
                if (!object[key].IsArray()) {
                    throw Exception(ERROR::INPUT, "Invalid " + std::string(key) + " array.");
                }

                for (rapidjson::SizeType i = 0; i < object[key].Size(); ++i) {
                    if (!object[key][i].IsInt64()) {
                        throw Exception(ERROR::INPUT, "Invalid " + std::string(key) + " value.");
                    }
                    amount.push_back(object[key][i].GetInt64());
                }
            }

            return amount;
        }

        inline Skills get_skills(const rapidjson::Value& object)
        {
            Skills skills;
            if (object.HasMember("skills")) {
                if (!object["skills"].IsArray()) {
                    throw Exception(ERROR::INPUT, "Invalid skills object.");
                }
                for (rapidjson::SizeType i = 0; i < object["skills"].Size(); ++i) {
                    if (!object["skills"][i].IsUint()) {
                        throw Exception(ERROR::INPUT, "Invalid skill value.");
                    }
                    skills.insert(object["skills"][i].GetUint());
                }
            }

            return skills;
        }

        inline Duration get_service(const rapidjson::Value& object)
        {
            Duration service = 0;
            if (object.HasMember("service")) {
                if (!object["service"].IsUint()) {
                    throw Exception(ERROR::INPUT, "Invalid service value.");
                }
                service = object["service"].GetUint();
            }
            return service;
        }

        inline bool valid_vehicle(const rapidjson::Value& v)
        {
            return v.IsObject() and v.HasMember("id") and v["id"].IsUint64();
        }

        inline TimeWindow get_time_window(const rapidjson::Value& tw)
        {
            if (!tw.IsArray() or tw.Size() < 2 or !tw[0].IsUint() or !tw[1].IsUint()) {
                throw Exception(ERROR::INPUT, "Invalid time-window.");
            }
            return TimeWindow(tw[0].GetUint(), tw[1].GetUint());
        }

        inline TimeWindow get_vehicle_time_window(const rapidjson::Value& v)
        {
            TimeWindow v_tw = TimeWindow();
            if (v.HasMember("time_window")) {
                v_tw = get_time_window(v["time_window"]);
            }
            return v_tw;
        }

        inline std::vector<TimeWindow> get_job_time_windows(const rapidjson::Value& j)
        {
            std::vector<TimeWindow> tws;
            if (j.HasMember("time_windows")) {
                if (!j["time_windows"].IsArray()) {
                    throw Exception(ERROR::INPUT,
                                    "Invalid time_windows value for job " + std::to_string(j["id"].GetUint64()) + ".");
                }

                std::transform(j["time_windows"].Begin(), j["time_windows"].End(), std::back_inserter(tws),
                               [](auto& tw) { return get_time_window(tw); });

                std::sort(tws.begin(), tws.end());
            } else {
                tws = std::vector<TimeWindow>(1, TimeWindow());
            }

            return tws;
        }

        Input parse(const CLArgs& cl_args)
        {
            // Custom input object embedding jobs, vehicles and matrix.
            Input input;
            input.set_geometry(cl_args.geometry);

            // Input json object.
            rapidjson::Document json_input;

            // Parsing input string to populate the input object.
            if (json_input.Parse(cl_args.input.c_str()).HasParseError()) {
                std::string error_msg = std::string(rapidjson::GetParseError_En(json_input.GetParseError())) +
                                        " (offset: " + std::to_string(json_input.GetErrorOffset()) + ")";
                throw Exception(ERROR::INPUT, error_msg);
            }

            // Main Checks for valid json input.
            if (!json_input.HasMember("jobs") or !json_input["jobs"].IsArray() or json_input["jobs"].Empty()) {
                throw Exception(ERROR::INPUT, "Invalid jobs.");
            }

            if (!json_input.HasMember("vehicles") or !json_input["vehicles"].IsArray() or
                json_input["vehicles"].Empty()) {
                throw Exception(ERROR::INPUT, "Invalid vehicles.");
            }

            // Used to make sure all vehicles have the same profile.
            std::string common_profile;

            // Switch input type: explicit matrix or using OSRM.
            if (json_input.HasMember("matrix")) {
                if (!json_input["matrix"].IsArray()) {
                    throw Exception(ERROR::INPUT, "Invalid matrix.");
                }

                // Load custom matrix while checking if it is square.
                rapidjson::SizeType matrix_size = json_input["matrix"].Size();

                Matrix<Cost> matrix_input(matrix_size);
                for (rapidjson::SizeType i = 0; i < matrix_size; ++i) {
                    if (!json_input["matrix"][i].IsArray() or (json_input["matrix"][i].Size() != matrix_size)) {
                        throw Exception(ERROR::INPUT, "Invalid matrix line " + std::to_string(i) + ".");
                    }
                    rapidjson::Document::Array mi = json_input["matrix"][i].GetArray();
                    for (rapidjson::SizeType j = 0; j < matrix_size; ++j) {
                        if (!mi[j].IsUint()) {
                            throw Exception(ERROR::INPUT, "Invalid matrix entry (" + std::to_string(i) + "," +
                                                            std::to_string(j) + ").");
                        }
                        Cost cost          = mi[j].GetUint();
                        matrix_input[i][j] = cost;
                    }
                }
                input.set_matrix(std::move(matrix_input));

                // Add all vehicles.
                for (rapidjson::SizeType i = 0; i < json_input["vehicles"].Size(); ++i) {
                    auto& json_vehicle = json_input["vehicles"][i];
                    if (!valid_vehicle(json_vehicle)) {
                        throw Exception(ERROR::INPUT, "Invalid vehicle at " + std::to_string(i) + ".");
                    }
                    auto v_id = json_vehicle["id"].GetUint();

                    // Check if vehicle has start_index or end_index.
                    bool  has_start_index = json_vehicle.HasMember("start_index");
                    Index start_index     = 0; // Initial value actually never used.
                    if (has_start_index) {
                        if (!json_vehicle["start_index"].IsUint()) {
                            throw Exception(ERROR::INPUT,
                                            "Invalid start_index for vehicle " + std::to_string(v_id) + ".");
                        }
                        start_index = json_vehicle["start_index"].GetUint();

                        if (matrix_size <= start_index) {
                            throw Exception(ERROR::INPUT, "start_index exceeding matrix size for vehicle" +
                                                            std::to_string(v_id) + ".");
                        }
                    }

                    bool has_start_coords = json_vehicle.HasMember("start");

                    bool  has_end_index = json_vehicle.HasMember("end_index");
                    Index end_index     = 0; // Initial value actually never used.
                    if (has_end_index) {
                        if (!json_vehicle["end_index"].IsUint()) {
                            throw Exception(ERROR::INPUT, "Invalid end_index for vehicle" + std::to_string(v_id) + ".");
                        }
                        end_index = json_vehicle["end_index"].GetUint();

                        if (matrix_size <= end_index) {
                            throw Exception(ERROR::INPUT,
                                            "end_index exceeding matrix size for vehicle" + std::to_string(v_id) + ".");
                        }
                    }

                    bool has_end_coords = json_vehicle.HasMember("end");

                    // Add vehicle to input
                    boost::optional<Location> start;
                    if (has_start_index) {
                        if (has_start_coords) {
                            start = boost::optional<Location>(
                              Location({start_index, parse_coordinates(json_vehicle, "start")}));
                        } else {
                            start = boost::optional<Location>(start_index);
                        }
                    }

                    boost::optional<Location> end;
                    if (has_end_index) {
                        if (has_end_coords) {
                            end =
                              boost::optional<Location>(Location({end_index, parse_coordinates(json_vehicle, "end")}));
                        } else {
                            end = boost::optional<Location>(end_index);
                        }
                    }

                    Vehicle current_v(v_id, start, end, get_amount(json_vehicle, "capacity"), get_skills(json_vehicle),
                                      get_vehicle_time_window(json_vehicle));

                    input.add_vehicle(current_v);

                    bool        has_profile     = json_vehicle.HasMember("profile");
                    std::string current_profile = (has_profile) ? get_string(json_vehicle, "profile") : DEFAULT_PROFILE;

                    if (common_profile.empty()) {
                        // First iteration only.
                        common_profile = current_profile;
                    } else {
                        // Check profile consistency.
                        if (current_profile != common_profile) {
                            throw Exception(ERROR::INPUT, "Mixed vehicle profiles in input.");
                        }
                    }
                }

                // Add the jobs
                for (rapidjson::SizeType i = 0; i < json_input["jobs"].Size(); ++i) {
                    auto& json_job = json_input["jobs"][i];
                    if (!json_job.IsObject()) {
                        throw Exception(ERROR::INPUT, "Invalid job.");
                    }
                    if (!json_job.HasMember("id") or !json_job["id"].IsUint64()) {
                        throw Exception(ERROR::INPUT, "Invalid id for job at " + std::to_string(i) + ".");
                    }
                    auto j_id = json_job["id"].GetUint64();
                    if (!json_job.HasMember("location_index") or !json_job["location_index"].IsUint()) {
                        throw Exception(ERROR::INPUT, "Invalid location_index for job " + std::to_string(j_id) + ".");
                    }
                    if (matrix_size <= json_job["location_index"].GetUint()) {
                        throw Exception(ERROR::INPUT,
                                        "location_index exceeding matrix size for job " + std::to_string(j_id) + ".");
                    }

                    auto     job_loc_index = json_job["location_index"].GetUint();
                    Location job_loc(job_loc_index);

                    if (json_job.HasMember("location")) {
                        job_loc = Location(job_loc_index, parse_coordinates(json_job, "location"));
                    }

                    Job current_job(j_id, job_loc, get_service(json_job), get_amount(json_job, "amount"),
                                    get_skills(json_job), get_job_time_windows(json_job));

                    input.add_job(current_job);
                }
            } else {
                // Adding vehicles and jobs only, matrix will be computed using
                // OSRM upon solving.

                // All vehicles.
                for (rapidjson::SizeType i = 0; i < json_input["vehicles"].Size(); ++i) {
                    auto& json_vehicle = json_input["vehicles"][i];
                    if (!valid_vehicle(json_vehicle)) {
                        throw Exception(ERROR::INPUT, "Invalid vehicle at " + std::to_string(i) + ".");
                    }

                    // Start def is a ugly workaround as using plain:
                    //
                    // boost::optional<Location> start;
                    //
                    // will raise a false positive -Wmaybe-uninitialized with gcc,
                    // see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=47679

                    auto start([]() -> boost::optional<Location> { return boost::none; }());
                    if (json_vehicle.HasMember("start")) {
                        start = boost::optional<Location>(parse_coordinates(json_vehicle, "start"));
                    }

                    boost::optional<Location> end;
                    if (json_vehicle.HasMember("end")) {
                        end = boost::optional<Location>(parse_coordinates(json_vehicle, "end"));
                    }

                    Vehicle current_v(json_vehicle["id"].GetUint(), start, end, get_amount(json_vehicle, "capacity"),
                                      get_skills(json_vehicle), get_vehicle_time_window(json_vehicle));

                    input.add_vehicle(current_v);

                    bool        has_profile     = json_vehicle.HasMember("profile");
                    std::string current_profile = (has_profile) ? get_string(json_vehicle, "profile") : DEFAULT_PROFILE;

                    if (common_profile.empty()) {
                        // First iteration only.
                        common_profile = current_profile;
                    } else {
                        // Check profile consistency.
                        if (current_profile != common_profile) {
                            throw Exception(ERROR::INPUT, "Mixed vehicle profiles in input.");
                        }
                    }
                }

                // Getting jobs.
                for (rapidjson::SizeType i = 0; i < json_input["jobs"].Size(); ++i) {
                    auto& json_job = json_input["jobs"][i];
                    if (!json_job.IsObject()) {
                        throw Exception(ERROR::INPUT, "Invalid job.");
                    }
                    if (!json_job.HasMember("id") or !json_job["id"].IsUint64()) {
                        throw Exception(ERROR::INPUT, "Invalid id for job at " + std::to_string(i) + ".");
                    }
                    auto j_id = json_job["id"].GetUint64();
                    if (!json_job.HasMember("location") or !json_job["location"].IsArray()) {
                        throw Exception(ERROR::INPUT, "Invalid location for job " + std::to_string(j_id) + ".");
                    }

                    Job current_job(j_id, parse_coordinates(json_job, "location"), get_service(json_job),
                                    get_amount(json_job, "amount"), get_skills(json_job),
                                    get_job_time_windows(json_job));

                    input.add_job(current_job);
                }
            }

            // Set relevant routing wrapper.
            std::unique_ptr<routing::Wrapper<Cost>> routing_wrapper;
            switch (cl_args.router) {
                case ROUTER::OSRM: {
                    // Use osrm-routed.
                    auto search = cl_args.servers.find(common_profile);
                    if (search == cl_args.servers.end()) {
                        throw Exception(ERROR::INPUT, "Invalid profile: " + common_profile + ".");
                    }
                    routing_wrapper = std::make_unique<routing::RoutedWrapper>(common_profile, search->second);
                } break;
                case ROUTER::LIBOSRM:
                    // Use libosrm.
                    try {
                        routing_wrapper = std::make_unique<routing::LibosrmWrapper>(common_profile);
                    } catch (const osrm::exception& e) {
                        throw Exception(ERROR::ROUTING, "Invalid shared memory region: " + common_profile);
                    }
                    break;
                case ROUTER::ORS:
                    // Use ORS http wrapper.
                    auto search = cl_args.servers.find(common_profile);
                    if (search == cl_args.servers.end()) {
                        throw Exception(ERROR::INPUT, "Invalid profile: " + common_profile + ".");
                    }
                    routing_wrapper = std::make_unique<routing::OrsHttpWrapper>(common_profile, search->second);
                    break;
            }
            input.set_routing(std::move(routing_wrapper));

            return input;
        }

    } // namespace io
} // namespace vroom
