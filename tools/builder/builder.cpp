#include <string>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <chrono>
#include "INIReader.h"

using namespace std;
namespace fs = std::filesystem;

vector<string> installedFiles;
vector<string> installedFolders;

void clear_build() {
  fs::remove_all("build");
}

void build_archive() {
  fs::create_directories("build/archive/pc/mod");
  fs::copy("wolvenkit/packed/archive/pc/mod/let_there_be_flight.archive",
           "build/archive/pc/mod/let_there_be_flight.archive",
           fs::copy_options::update_existing);
  cout << "Copied archive file: let_there_be_flight.archive" << endl;

  installedFiles.push_back("archive/pc/mod/let_there_be_flight.archive");
}

void build_redscript() {
  fs::create_directories("build/r6/scripts");
  stringstream contents;
  contents << "// Let There Be Flight" << endl
           << "// (C) 2022 Jack Humbert" << endl
           << "// https://github.com/jackhumbert/let_there_be_flight" << endl
           << "// This file was automatically generated on "
           << std::chrono::system_clock::now() << endl
           << endl;
  for (const auto &entry :
       fs::directory_iterator("src/redscript/let_there_be_flight")) {
    ifstream file(entry.path());
    if (file) {
      contents << "// " << entry.path().filename().string() << endl
               << endl
               << file.rdbuf() << endl
               << endl;
      cout << "Added redscript file: " << entry.path().filename().string()
           << endl;
      file.close();
    } else {
      cout << "Could not read file: " << entry.path().filename().string()
           << endl;
    }
  }
  ofstream compiledFile("build/r6/scripts/let_there_be_flight.reds");
  installedFiles.push_back("r6/scripts/let_there_be_flight.reds");
  compiledFile << contents.rdbuf();

  compiledFile.close();

  fs::copy("src/redscript/codeware", "build/r6/scripts/codeware",
           fs::copy_options::update_existing | fs::copy_options::recursive);
  installedFolders.push_back("r6/scripts/codeware");
}

void build_tweaks() {
  fs::create_directories("build/r6/tweaks");
  stringstream contents;
  contents << "# Let There Be Flight" << endl
           << "# (C) 2022 Jack Humbert" << endl
           << "# https://github.com/jackhumbert/let_there_be_flight" << endl
           << "# This file was automatically generated on "
           << std::chrono::system_clock::now() << endl
           << endl;
  for (const auto &entry :
       fs::directory_iterator("src/tweaks")) {
    ifstream file(entry.path());
    if (file) {
      contents << "# " << entry.path().filename().string() << endl
               << endl
               << file.rdbuf() << endl
               << endl;
      cout << "Added tweak file: " << entry.path().filename().string()
           << endl;
      file.close();
    } else {
      cout << "Could not read file: " << entry.path().filename().string()
           << endl;
    }
  }
  ofstream compiledFile("build/r6/tweaks/let_there_be_flight.yaml");
  installedFiles.push_back("r6/tweaks/let_there_be_flight.yaml");
  compiledFile << contents.rdbuf();

  compiledFile.close();
}

void build_fmod() {
  fs::create_directories("build/red4ext/plugins/let_there_be_flight");
  vector<string> files;
  for (const auto &entry : fs::directory_iterator("fmod_studio/API")) {
    fs::copy(entry.path(), "build/red4ext/plugins/let_there_be_flight/" + entry.path().filename().string(),
             fs::copy_options::update_existing);
    cout << "Copied FMOD file: " << entry.path().filename().string() << endl;
  }

  fs::create_directories("build/red4ext/plugins/let_there_be_flight");
  for (const auto &entry : fs::directory_iterator("fmod_studio/Build/Desktop")) {
    fs::copy(entry.path(), "build/red4ext/plugins/let_there_be_flight/" + entry.path().filename().string(),
             fs::copy_options::update_existing);
    cout << "Copied FMOD file: " << entry.path().filename().string() << endl;
  }
  installedFolders.push_back("red4ext/plugins/let_there_be_flight");
}

void build_red4ext() {
  fs::copy("src/red4ext/build/release/bin/flight_control.dll", "build/red4ext/plugins/let_there_be_flight/let_there_be_flight.dll", fs::copy_options::update_existing);
  cout << "Copied RED4ext file: let_there_be_flight.dll" << endl;
}

void build_input() {
  fs::create_directories("build/r6/input");
  fs::copy("src/input/let_there_be_flight.xml",
           "build/r6/input/let_there_be_flight.xml",
           fs::copy_options::update_existing);
  cout << "Copied input file: let_there_be_flight.xml" << endl;

  installedFiles.push_back("r6/input/let_there_be_flight.xml");
}

void build_info() {
  fs::copy("readme.md", "build/red4ext/plugins/let_there_be_flight/readme.md",
           fs::copy_options::update_existing);
  cout << "Copied input file: readme.md" << endl;
  fs::copy("license.md", "build/red4ext/plugins/let_there_be_flight/license.md",
           fs::copy_options::update_existing);
  cout << "Copied input file: license.md" << endl;
}

void build_uninstaller() {
  stringstream contents;
  contents << "DEL /S /Q";
  for (const auto &file : installedFiles) {
    contents << " ..\\..\\..\\" << fs::path(file).make_preferred().string();
  }
  contents << endl << "@RD /S /Q";
  for (const auto &folder : installedFolders) {
    contents << " ..\\..\\..\\" << fs::path(folder).make_preferred().string();
  }

  ofstream uninstaller(
      "build/red4ext/plugins/let_there_be_flight/uninstall.bat");
  uninstaller << contents.rdbuf();
  uninstaller.close();
}

int main() {

  //auto reader = INIReader::INIReader("build.ini");

  //if (reader.ParseError() != 0) {
  //  cout << "Couldn't read build.ini" << endl;
  //} else {
  //  auto project_name_safe = reader.Get("mod", "project_name_safe", "");
  //  auto sections = reader.Sections();
  //  for (const auto &section : sections) {
  //    cout << "[" << section << "]" << endl;
  //    if (section == "mod" || section == "core")
  //      continue;
  //    auto path = reader.Get(section, "path", "");
  //    if (path == "")
  //      continue;
  //    auto build_path = "build/" + path;
  //    fs::create_directories(build_path);
  //    auto file = reader.Get(section, "file", "");
  //    if (file != "") {
  //      auto file_path = path + fs::path(file).filename().string();
  //      fs::copy(file, file_path, fs::copy_options::update_existing);
  //      cout << "Copied " << section << " file : " << file << endl;
  //      installedFiles.push_back(file_path);
  //    }
  //    auto folder = reader.Get(section, "folder", "");
  //    if (folder != "") {
  //      cout << "Folder: " << build_path << endl;
  //      for (const auto &entry : fs::directory_iterator(folder)) {
  //        fs::copy(entry.path(), build_path + entry.path().filename().string(),
  //                 fs::copy_options::update_existing);
  //        cout << "Copied " << section
  //             << " file : " << entry.path().filename().string() << endl;
  //        installedFolders.push_back(path);
  //      }
  //    }
  //  }
  //}

  //clear_build();
  build_archive();
  build_redscript();
  build_tweaks();
  build_fmod();
  build_red4ext();
  build_input();
  build_info();
  build_uninstaller();

	return EXIT_SUCCESS;
}