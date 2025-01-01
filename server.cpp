#include "crow.h"
#include <asio.hpp>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <mysqlx/xdevapi.h>


// // request data we are then processing, middleware
// // struct ContextData{

// //     // local data where we store the parsed data, intermediate for the other service to handle
// //     struct context{};

// //     // takes input to api and standerdises it
// //     void before_handle(crow::request &req, crow::response &res, context &ctx){

// //     }

// //     // after we are finished with intermediate service we then send it back to client in this format
// //     void after_handle(crow::request &req, crow::response &res, context &ctx){

// //     }
// // };



int main(){

    // Instance of the api server I'm creating
    // crow::App<ContextData> server;
    crow::SimpleApp server; // testing


    // Route for root
    CROW_ROUTE(server, "/")
    ([](){
        return "this is root";
    });

    
    // MACHINE LEARNING ROUTING
    
    // Route for uploading video processing - machine learning

    CROW_ROUTE(server, "/uploading_to_ml_model").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req) {
        try {

            // check the type of content we are recieveing set by client
            std::cout << "Received request: " << req.get_header_value("Content-Type") << std::endl;
            
            // if we don't find anything we get a value of -1 which is npos, negative position , position not found
            // if we find the string we are looking for we get a value greater than 0
            if (req.get_header_value("Content-Type").find("multipart/form-data") == std::string::npos) {
                return crow::response(400, "Didn't recieve type multipart/form-data");
            }

            // we are parsing the request data as a multipart message, puts the parts in a vector
            crow::multipart::message msg(req);

            // if we dont have any parts that means we have recieved no data just a call to the api
            if (msg.parts.empty()) {
                return crow::response(400, "No parts in the request.");
            }

            // we are setting the file path and name to store the data
            const std::string temp_file = "uploaded_video.mov";

            // we are opening the file at temp_file and writing binary data into it
            std::ofstream out(temp_file, std::ios::binary);


            // if we don't have a file to open we return a 500 error
            if (!out) {
                return crow::response(500, "Failed to open file for writing.");
            }

            // we are iterating over the different parts of the message to check if the content type is video quicktime or if it is empty
            // we then give a error if we don't get expected type
            // otherwise we then write the data to the file set by the size of the body being put in
            for (const auto& part : msg.parts) {
                const std::string content_type = part.get_header_object("Content-Type").value;
                if (content_type.empty() || content_type != "video/quicktime") {
                    return crow::response(400, "Invalid file type.");
                }
                out.write(part.body.data(), part.body.size());
            }

            // we then close the file after this is completed
            out.close();


            // SUCCESS!!!
            return crow::response(200, "Video uploaded successfully.");
            
        } catch (const std::exception& e) {

            // this is if we get a error while we process it such as segmentation fault ect
            return crow::response(500, std::string("Server error: ") + e.what());
        }
    });


    // Route for retrieving the video processing

    CROW_ROUTE(server, "/retrieving_the_ml_model").methods(crow::HTTPMethod::GET)
    ([](){
        return "this will return the highlight timing and court coverage";
    }); 


    




    // SETTING THE ROUTES FOR VIDEO DATA

    // Route for setting video data in the data base
    CROW_ROUTE(server, "/video_meta_storing").methods(crow::HTTPMethod::PUT)
    ([](){
        return "this is where we store data for video data"; 
    });

    // Route for getting video data from the data base
    CROW_ROUTE(server, "/video_meta_history").methods(crow::HTTPMethod::GET)
    ([](){
        // handles what we send back to the client
        return "this is where we call on the data base for video data";
    });




    // SETTING THE ROUTES FOR USER DATA

    // Route for setting user data in the data base
    CROW_ROUTE(server, "/user_meta_retrieving").methods(crow::HTTPMethod::PUT)
    ([](const crow::request& req){
    try {
        
        if (req.body == ""){
            return crow::response(400, "");
        }

        std::cout << req.body << std::endl;
        mysqlx::Session session("127.0.0.1", 33060, "root", "test");
        mysqlx::Schema schema = session.getSchema("restapi");
        mysqlx::Table table = schema.getTable("Account");
        mysqlx::RowResult result = table.select("username", "password")
                                         .where("username = :username")
                                         .bind("username", req.body)
                                         .execute();

        std::string response = "";

        if (auto row = result.fetchOne()) {
            response += row[0].get<std::string>() + "," + row[1].get<std::string>();
        }
        std::cout << response << std::endl;
        return crow::response(response);

    } catch (const mysqlx::Error &err) {
        return crow::response(500, "MySQL Error: " + std::string(err.what()));
    } catch (std::exception &ex) {
        return crow::response(500, "Standard Exception: " + std::string(ex.what()));
    } catch (...) {
        return crow::response(500, "Unknown Error");
    }
    });

    // Route for storing user data in the data base
    CROW_ROUTE(server, "/user_meta_storing").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        try {
            if (req.body == ""){
                return crow::response(400, "");
            }

            std::string body = req.body;
            size_t comma = body.find(',');
            if (comma == std::string::npos) {
                return crow::response(400, "Invalid format");
            }
            
            std::string username = body.substr(0, comma);
            std::string password = body.substr(comma + 1);

            mysqlx::Session session("127.0.0.1", 33060, "root", "test");
            mysqlx::Schema schema = session.getSchema("restapi");
            mysqlx::Table table = schema.getTable("Account");
            
            table.insert("username", "password")
                 .values(username, password)
                 .execute();

            return crow::response(200, "User created successfully");

        } catch (const mysqlx::Error &err) {
            return crow::response(500, "MySQL Error: " + std::string(err.what()));
        } catch (std::exception &ex) {
            return crow::response(500, "Standard Exception: " + std::string(ex.what()));
        } catch (...) {
            return crow::response(500, "Unknown Error");
        }
    });


    CROW_ROUTE(server, "/user_meta_storingclips").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        try {
            if (req.body.empty()) {
                return crow::response(400, "Empty request body");
            }

            // Parse username and clipId from request body
            std::string body = req.body;
            size_t comma = body.find(',');
            if (comma == std::string::npos) {
                return crow::response(400, "Invalid format: expected 'username,clipId'");
            }
            
            std::string username = body.substr(0, comma);
            std::string clipId = body.substr(comma + 1);

            mysqlx::Session session("127.0.0.1", 33060, "root", "test");
            mysqlx::Schema schema = session.getSchema("restapi");
            mysqlx::Table table = schema.getTable("Account");

            // First, get the existing clips (if any)
            mysqlx::RowResult result = table.select("clipskeys")
                                           .where("username = :username")
                                           .bind("username", username)
                                           .execute();

            if (auto row = result.fetchOne()) {
                std::string existingClips;
                // Check if the value is NULL
                if (!row[0].isNull()) {
                    existingClips = row[0].get<std::string>();
                }
                
                std::string updatedClips = existingClips.empty() ? clipId : existingClips + "," + clipId;
                
                table.update()
                     .set("clipskeys", updatedClips)
                     .where("username = :username")
                     .bind("username", username)
                     .execute();
            } else {
                // If no existing clips, just set the new clipId
                table.update()
                     .set("clipskeys", clipId)
                     .where("username = :username")
                     .bind("username", username)
                     .execute();
            }

            return crow::response(200, "Clips updated successfully");

        } catch (const mysqlx::Error &err) {
            return crow::response(500, "MySQL Error: " + std::string(err.what()));
        } catch (std::exception &ex) {
            return crow::response(500, "Standard Exception: " + std::string(ex.what()));
        } catch (...) {
            return crow::response(500, "Unknown Error");
        }
    });

    CROW_ROUTE(server, "/user_meta_retrievingclips").methods(crow::HTTPMethod::PUT)
    ([](const crow::request& req){
        try {
            if (req.body == ""){
                return crow::response(400, "");
            }
            mysqlx::Session session("127.0.0.1", 33060, "root", "test");
            mysqlx::Schema schema = session.getSchema("restapi");
            mysqlx::Table table = schema.getTable("Account");

            mysqlx::RowResult result = table.select("clipskeys")
                                           .where("username = :username")
                                           .bind("username", req.body)
                                           .execute();
            std::string response = "";
            if (auto row = result.fetchOne()) {
                response += row[0].get<std::string>();
            }
            return crow::response(response);
            
        } catch (const mysqlx::Error &err) {
            return crow::response(500, "MySQL Error: " + std::string(err.what()));
        }
    });

    CROW_ROUTE(server, "/user_meta_deletingclips").methods(crow::HTTPMethod::DELETE)
    ([](const crow::request& req){
        try {
            if (req.body.empty()) {
                return crow::response(400, "Empty request body");
            }

            // Parse username and clipId from request body
            std::string body = req.body;
            size_t comma = body.find(',');
            if (comma == std::string::npos) {
                return crow::response(400, "Invalid format: expected 'username,clipId'");
            }
            
            std::string username = body.substr(0, comma);
            std::string clipToDelete = body.substr(comma + 1);

            mysqlx::Session session("127.0.0.1", 33060, "root", "test");
            mysqlx::Schema schema = session.getSchema("restapi");
            mysqlx::Table table = schema.getTable("Account");

            mysqlx::RowResult result = table.select("clipskeys")
                                           .where("username = :username")
                                           .bind("username", username)
                                           .execute();

            if (auto row = result.fetchOne()) {
                if (!row[0].isNull()) {
                    std::string clips = row[0].get<std::string>();
                    std::string updatedClips;
                    size_t start = 0;
                    size_t end = 0;
                    
                    while ((end = clips.find(',', start)) != std::string::npos) {
                        std::string clip = clips.substr(start, end - start);
                        if (clip != clipToDelete) {
                            if (!updatedClips.empty()) updatedClips += ",";
                            updatedClips += clip;
                        }
                        start = end + 1;
                    }
                    
                    std::string lastClip = clips.substr(start);
                    if (lastClip != clipToDelete) {
                        if (!updatedClips.empty()) updatedClips += ",";
                        updatedClips += lastClip;
                    }

                    table.update()
                         .set("clipskeys", updatedClips)
                         .where("username = :username")
                         .bind("username", username)
                         .execute();

                    return crow::response(200, "Clip deleted successfully");
                }
            }
            
            return crow::response(404, "User or clips not found");

        } catch (const mysqlx::Error &err) {
            return crow::response(500, "MySQL Error: " + std::string(err.what()));
        } catch (std::exception &ex) {
            return crow::response(500, "Standard Exception: " + std::string(ex.what()));
        } catch (...) {
            return crow::response(500, "Unknown Error");
        }
    });




    // Runs server, set default port to default (80) and used threading to handle multiple requests
    server.port(80).multithreaded().run();

    // Success
    return 0;
}