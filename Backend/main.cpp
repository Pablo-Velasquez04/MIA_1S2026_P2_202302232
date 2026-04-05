//Backend/main.cpp
#include "httplib.h"
#include "CommandParser/CommandParser.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp> 

using json = nlohmann::json;
namespace fs = std::filesystem;

int main() {
    srand(time(nullptr));
    httplib::Server server;

    //CORS — necesario para Next.js 
    auto setCORS = [](httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin",  "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
    };

    server.Options(".*", [&setCORS](const httplib::Request&, 
                                     httplib::Response& res) {
        setCORS(res);
        res.status = 204;
    });

    //GET /api/health 
    server.Get("/api/health", [&setCORS](const httplib::Request&, 
                                          httplib::Response& res) {
        setCORS(res);
        json body = {{"status", "ok"}, {"message", "ExtreamFS backend running"}};
        res.set_content(body.dump(), "application/json");
    });

    //POST /api/execute
    //Body: { "command": "mkdisk -size=5 -unit=M -path=/tmp/disco.mia" }
    //Response: { "success": true, "output": "..." }
    server.Post("/api/execute", [&setCORS](const httplib::Request& req,
                                            httplib::Response& res) {
        setCORS(res);
        try {
            auto body = json::parse(req.body);
            std::string command = body["command"];

            //Ignorar comentarios y vacíos
            std::string trimmed = command;
            while (!trimmed.empty() && isspace(trimmed.front()))
                trimmed.erase(trimmed.begin());

            if (trimmed.empty() || trimmed[0] == '#') {
                json response = {{"success", true}, {"output", command}};
                res.set_content(response.dump(), "application/json");
                return;
            }

            //Ejecutar el comando
            std::string output = processLine(command);

            //Detectar si es error
            bool isError = output.rfind("Error", 0) == 0;

            json response = {
                {"success", !isError},
                {"output",  output},
            };

            if (isError) {
                response["error"] = output;
            }

            // Si es reporte, agregar el path
            if (command.find("rep") != std::string::npos || 
                command.find("REP") != std::string::npos) {
                auto pathMatch = command.find("-path=");
                if (pathMatch != std::string::npos) {
                    std::string reportPath = command.substr(pathMatch + 6);
                    // Quitar comillas si las hay
                    if (!reportPath.empty() && reportPath[0] == '"')
                        reportPath = reportPath.substr(1, reportPath.find('"', 1) - 1);
                    else {
                        auto spacePos = reportPath.find(' ');
                        if (spacePos != std::string::npos)
                            reportPath = reportPath.substr(0, spacePos);
                    }
                    response["reportPath"] = reportPath;
                }
            }

            res.set_content(response.dump(), "application/json");

        } catch (const std::exception& e) {
            json error = {{"success", false}, {"error", e.what()}};
            res.set_content(error.dump(), "application/json");
            res.status = 400;
        }
    });

    //GET /api/reports/:path 
    //El frontend pide la imagen por path codificado en URL
    server.Get("/api/reports/(.*)", [&setCORS](const httplib::Request& req,
                                                httplib::Response& res) {
        setCORS(res);
        std::string encodedPath = req.matches[1];

        //Decodificar URL encoding (%2F → /)
        std::string filePath;
        for (size_t i = 0; i < encodedPath.size(); i++) {
            if (encodedPath[i] == '%' && i + 2 < encodedPath.size()) {
                int hex = std::stoi(encodedPath.substr(i + 1, 2), nullptr, 16);
                filePath += (char)hex;
                i += 2;
            } else {
                filePath += encodedPath[i];
            }
        }

        if (!fs::exists(filePath)) {
            res.status = 404;
            res.set_content("Reporte no encontrado: " + filePath, "text/plain");
            return;
        }

        //Leer el archivo
        std::ifstream file(filePath, std::ios::binary);
        std::string content((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());

        //Determinar content-type
        std::string ext = fs::path(filePath).extension().string();
        std::string contentType = "image/jpeg";
        if (ext == ".png") contentType = "image/png";
        if (ext == ".pdf") contentType = "application/pdf";
        if (ext == ".txt") contentType = "text/plain";

        res.set_content(content, contentType);
    });

    std::cout << "ExtreamFS backend corriendo en http://localhost:8080\n";
    std::cout << "Endpoints:\n";
    std::cout << "  GET  /api/health\n";
    std::cout << "  POST /api/execute\n";
    std::cout << "  GET  /api/reports/:path\n";

    server.listen("0.0.0.0", 8080);
    return 0;
}