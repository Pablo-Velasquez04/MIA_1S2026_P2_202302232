//Backend/main.cpp
#include "httplib.h"
#include "CommandParser/CommandParser.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp> 
#include "Estructuras/Structs.h"
#include "Mounting/Mounting.h"
#include "FilesFolders/FilesFolders.h"

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

    // --- EXPLORER ENDPOINTS ---

    // GET /api/explorer/disks
    server.Get("/api/explorer/disks", [&setCORS](const httplib::Request&, httplib::Response& res) {
        setCORS(res);
        try {
            json response = json::array();
            auto mounted = getMountedPartitions();
            std::map<std::string, int> diskCounts;
            for (const auto& mp : mounted) {
                diskCounts[mp.Path]++;
            }
            
            for (const auto& kv : diskCounts) {
                std::string path = kv.first;
                std::fstream file(path, std::ios::in | std::ios::binary);
                if (!file.is_open()) continue;
                
                MBR mbr;
                file.seekg(0, std::ios::beg);
                file.read(reinterpret_cast<char*>(&mbr), sizeof(MBR));
                file.close();
                
                std::string name = fs::path(path).filename().string();
                
                response.push_back({
                    {"path", path},
                    {"name", name},
                    {"size", mbr.MbrSize},
                    {"fit", std::string(1, mbr.Fit)},
                    {"mountedCount", kv.second}
                });
            }
            res.set_content(response.dump(), "application/json");
        } catch (const std::exception& e) {
            json error = {{"success", false}, {"error", e.what()}};
            res.set_content(error.dump(), "application/json");
            res.status = 500;
        }
    });

    // GET /api/explorer/partitions
    server.Get("/api/explorer/partitions", [&setCORS](const httplib::Request& req, httplib::Response& res) {
        setCORS(res);
        try {
            if (!req.has_param("diskPath")) {
                res.status = 400; res.set_content(json({{"error", "Missing diskPath"}}).dump(), "application/json"); return;
            }
            std::string diskPath = req.get_param_value("diskPath");
            
            std::fstream file(diskPath, std::ios::in | std::ios::binary);
            if (!file.is_open()) {
                res.status = 404; res.set_content(json({{"error", "Disk file not found"}}).dump(), "application/json"); return;
            }
            
            MBR mbr;
            file.read(reinterpret_cast<char*>(&mbr), sizeof(MBR));
            
            auto mounted = getMountedPartitions();
            auto isMounted = [&](const std::string& name) -> std::string {
                for (const auto& mp : mounted) {
                    if (mp.Path == diskPath && std::string(mp.Name) == name) return mp.Id;
                }
                return "";
            };
            
            json partitions = json::array();
            for (int i = 0; i < 4; i++) {
                if (mbr.Partitions[i].Size > 0) {
                    std::string pName(mbr.Partitions[i].Name);
                    std::string id = isMounted(pName);
                    
                    partitions.push_back({
                        {"name", pName},
                        {"size", mbr.Partitions[i].Size},
                        {"type", std::string(1, mbr.Partitions[i].Type)},
                        {"fit", std::string(1, mbr.Partitions[i].Fit)},
                        {"status", id.empty() ? "Unmounted" : "Mounted"},
                        {"id", id}
                    });
                    
                    if (mbr.Partitions[i].Type == 'E') {
                        int ebrStart = mbr.Partitions[i].Start;
                        while (ebrStart != -1) {
                            EBR ebr;
                            file.seekg(ebrStart, std::ios::beg);
                            file.read(reinterpret_cast<char*>(&ebr), sizeof(EBR));
                            if (ebr.Size > 0 && ebr.Mount != 0 && strnlen(ebr.Name, 16) > 0) {
                                std::string logName(ebr.Name);
                                std::string logId = isMounted(logName);
                                partitions.push_back({
                                    {"name", logName},
                                    {"size", ebr.Size},
                                    {"type", "L"},
                                    {"fit", std::string(1, ebr.Fit)},
                                    {"status", logId.empty() ? "Unmounted" : "Mounted"},
                                    {"id", logId}
                                });
                            }
                            if (ebr.Next == -1 || ebr.Next == 0) break;
                            ebrStart = ebr.Next;
                        }
                    }
                }
            }
            file.close();
            res.set_content(partitions.dump(), "application/json");
        } catch (const std::exception& e) {
            json error = {{"error", e.what()}};
            res.set_content(error.dump(), "application/json");
            res.status = 500;
        }
    });

    // GET /api/explorer/dir
    server.Get("/api/explorer/dir", [&setCORS](const httplib::Request& req, httplib::Response& res) {
        setCORS(res);
        try {
            std::string id = req.has_param("id") ? req.get_param_value("id") : "";
            std::string reqPath = req.has_param("path") ? req.get_param_value("path") : "";
            if (id.empty() || reqPath.empty()) { res.status = 400; return; }
            
            auto mounted = getMountedPartitions();
            MountedPartition targetMp;
            bool found = false;
            for (const auto& mp : mounted) {
                if (mp.Id == id) { targetMp = mp; found = true; break; }
            }
            if (!found) { res.status = 404; return; }
            
            std::fstream disk(targetMp.Path, std::ios::in | std::ios::binary);
            if (!disk.is_open()) { res.status = 500; return; }
            
            SuperBloque sb;
            long sbStart = targetMp.Start;
            if (targetMp.Type == 'L') sbStart += sizeof(EBR);
            
            disk.seekg(sbStart, std::ios::beg);
            disk.read(reinterpret_cast<char*>(&sb), sizeof(SuperBloque));
            
            if (sb.Magic != 0xEF53) { disk.close(); res.status = 400; return; }
            
            // Ignoramos validacion de sesion (pasando root = 1) porque es solo lectura visual
            int inodeIdx = ffResolvePath(disk, sb, reqPath, false, 1, 1);
            if (inodeIdx < 0) { disk.close(); res.status = 404; return; }
            
            Inode dirInode;
            ffReadInode(disk, sb, inodeIdx, dirInode);
            if (dirInode.Type != '0') { disk.close(); res.status = 400; return; }
            
            json contents = json::array();
            auto processEntry = [&](int blockIdx) {
                FolderBlock fb;
                if (!ffReadFolderBlock(disk, sb, blockIdx, fb)) return;
                for (int e = 0; e < 4; e++) {
                    if (fb.content[e].Inodo != -1) {
                        std::string entryName(fb.content[e].Name, strnlen(fb.content[e].Name, 12));
                        if (entryName == "." || entryName == "..") continue;
                        
                        Inode child;
                        if (ffReadInode(disk, sb, fb.content[e].Inodo, child)) {
                            std::string perms(child.Perm, 3);
                            contents.push_back({
                                {"name", entryName},
                                {"type", child.Type == '1' ? "file" : "folder"},
                                {"perms", perms},
                                {"inodo", fb.content[e].Inodo},
                                {"size", child.Size},
                                {"ctime", child.Ctime}
                            });
                        }
                    }
                }
            };
            
            // Usando solo lectura basica, limitamos a 12 de momento u obtener también indirectos
            for (int i = 0; i < 12; i++) {
                if (dirInode.Block[i] != -1) processEntry(dirInode.Block[i]);
            }
            
            disk.close();
            res.set_content(contents.dump(), "application/json");
        } catch (const std::exception& e) {
            json error = {{"error", e.what()}};
            res.set_content(error.dump(), "application/json");
            res.status = 500;
        }
    });

    // GET /api/explorer/file
    server.Get("/api/explorer/file", [&setCORS](const httplib::Request& req, httplib::Response& res) {
        setCORS(res);
        try {
            std::string id = req.has_param("id") ? req.get_param_value("id") : "";
            std::string reqPath = req.has_param("path") ? req.get_param_value("path") : "";
            if (id.empty() || reqPath.empty()) { res.status = 400; return; }
            
            auto mounted = getMountedPartitions();
            MountedPartition targetMp;
            bool found = false;
            for (const auto& mp : mounted) {
                if (mp.Id == id) { targetMp = mp; found = true; break; }
            }
            if (!found) { res.status = 404; return; }
            
            std::fstream disk(targetMp.Path, std::ios::in | std::ios::binary);
            if (!disk.is_open()) { res.status = 500; return; }
            
            SuperBloque sb;
            long sbStart = targetMp.Start;
            if (targetMp.Type == 'L') sbStart += sizeof(EBR);
            
            disk.seekg(sbStart, std::ios::beg);
            disk.read(reinterpret_cast<char*>(&sb), sizeof(SuperBloque));
            
            if (sb.Magic != 0xEF53) { disk.close(); res.status = 400; return; }
            
            int inodeIdx = ffResolvePath(disk, sb, reqPath, false, 1, 1);
            if (inodeIdx < 0) { disk.close(); res.status = 404; return; }
            
            Inode fileInode;
            ffReadInode(disk, sb, inodeIdx, fileInode);
            if (fileInode.Type != '1') { disk.close(); res.status = 400; return; }
            
            std::string contentTotal = "";
            for (int i = 0; i < 12; i++) {
                if (fileInode.Block[i] != -1) {
                    FileBlock fb;
                    if (ffReadFileBlock(disk, sb, fileInode.Block[i], fb)) {
                        contentTotal += std::string(fb.content, strnlen(fb.content, 64));
                    }
                }
            }
            
            disk.close();
            res.set_content(json({{"content", contentTotal}}).dump(), "application/json");
        } catch (const std::exception& e) {
            json error = {{"error", e.what()}};
            res.set_content(error.dump(), "application/json");
            res.status = 500;
        }
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

    //POST /api/execute-script
    //Ejecuta múltiples comandos en lote, reiniciando el estado global previamente.
    server.Post("/api/execute-script", [&setCORS](const httplib::Request& req,
                                                   httplib::Response& res) {
        setCORS(res);
        try {
            auto body = json::parse(req.body);
            std::vector<std::string> commands = body["commands"];

            // Removido a petición para preservar estado RAM entre ejecuciones: 
            // processLine("execute");

            json responses = json::array();
            for (const auto& command : commands) {
                std::string trimmed = command;
                while (!trimmed.empty() && isspace(trimmed.front()))
                    trimmed.erase(trimmed.begin());

                if (trimmed.empty() || trimmed[0] == '#') {
                    responses.push_back({{"success", true}, {"output", command}});
                    continue;
                }

                std::string output = processLine(command);
                bool isError = output.rfind("Error", 0) == 0;

                json cmdRes = {
                    {"success", !isError},
                    {"output",  output},
                };
                if (isError) cmdRes["error"] = output;

                if (command.find("rep") != std::string::npos || 
                    command.find("REP") != std::string::npos) {
                    auto pathMatch = command.find("-path=");
                    if (pathMatch != std::string::npos) {
                        std::string reportPath = command.substr(pathMatch + 6);
                        if (!reportPath.empty() && reportPath[0] == '"')
                            reportPath = reportPath.substr(1, reportPath.find('"', 1) - 1);
                        else {
                            auto spacePos = reportPath.find(' ');
                            if (spacePos != std::string::npos)
                                reportPath = reportPath.substr(0, spacePos);
                        }
                        cmdRes["reportPath"] = reportPath;
                    }
                }
                responses.push_back(cmdRes);
            }

            res.set_content(responses.dump(), "application/json");

        } catch (const std::exception& e) {
            json error = {{"success", false}, {"error", e.what()}};
            res.set_content(error.dump(), "application/json");
            res.status = 400;
        }
    });

    std::cout << "ExtreamFS backend corriendo en http://localhost:8080\n";
    std::cout << "Endpoints:\n";
    std::cout << "  GET  /api/health\n";
    std::cout << "  POST /api/execute\n";
    std::cout << "  GET  /api/reports/:path\n";
    
    server.listen("0.0.0.0", 8080);
    return 0;
}