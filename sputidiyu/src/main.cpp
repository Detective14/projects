#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <regex>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

struct User {
    std::string login;
    std::string email;
    std::string password;
    std::string name;
    std::string birth;
};

static std::string trim(const std::string& s) {
    const auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    const auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::vector<User> loadUsers(const std::string& filename) {
    std::vector<User> users;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open " << filename << ". Trying ../db.txt" << std::endl;
        file.open("../db.txt");
    }
    if (!file.is_open()) {
        std::cerr << "Failed to open db.txt in both build and parent directory." << std::endl;
        return users;
    }

    std::string line;
    std::regex pattern("User: *(.+?) *\\| *Email: *(.+?) *\\| *Pass: *(.+?) *\\| *Name: *(.+?) *\\| *Birth: *(.+?)$");
    std::smatch matches;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        std::cout << "Processing line: '" << line << "'" << std::endl;
        if (std::regex_match(line, matches, pattern)) {
            std::cout << "Matches found: " << matches.size() << std::endl;
            users.push_back({trim(matches[1].str()), trim(matches[2].str()), trim(matches[3].str()), trim(matches[4].str()), trim(matches[5].str())});
        } else {
            std::cout << "No match for line" << std::endl;
        }
    }

    std::cerr << "Loaded " << users.size() << " users from database." << std::endl;
    for (const auto& user : users) {
        std::cerr << "User read: login='" << user.login << "', email='" << user.email << "', pass='" << user.password << "', name='" << user.name << "'" << std::endl;
    }
    return users;
}

void saveUsers(const std::string& filename, const std::vector<User>& users) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open " << filename << " for writing." << std::endl;
        return;
    }
    for (const auto& user : users) {
        file << "User: " << user.login << " | Email: " << user.email << " | Pass: " << user.password << " | Name: " << user.name << " | Birth: " << user.birth << "\n";
    }
    file.close();
}

bool authenticate(const std::vector<User>& users, const std::string& email, const std::string& password, std::string& login) {
    std::cout << "Users size in authenticate: " << users.size() << std::endl;
    std::cout << "Attempting login with email/login: '" << email << "', password: '" << password << "'" << std::endl;
    for (const auto& user : users) {
        std::cout << "Checking user: email='" << user.email << "', login='" << user.login << "', pass='" << user.password << "'" << std::endl;
        if ((user.email == email || user.login == email) && user.password == password) {
            login = user.login;
            std::cout << "Login successful for user: " << login << std::endl;
            return true;
        }
    }
    std::cout << "Login failed" << std::endl;
    return false;
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Electronic Diary", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsLight();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    std::vector<User> users = loadUsers("/home/trev/Dev/College/sputidiyu/db.txt");

    bool logged_in = false;
    std::string current_user;
    char login_email[256] = "";
    char login_password[256] = "";
    std::string error_message;

    std::vector<std::string> sections = {"Grades", "Schedule", "Tasks"};
    int current_section = 0;

    // Sample data for grades
    std::map<std::string, std::vector<int>> grades = {
        {"Mathematics", {5, 4, 5}},
        {"Physics", {4, 5}},
        {"Computer Science", {5, 5, 4}}
    };

    // Profile settings
    char profile_name[256] = "Name Surname";
    char profile_birthdate[256] = "01.01.2000";
    std::string profile_email = "email@example.com";
    bool show_profile_window = false;
    bool show_logout_confirm = false;
    bool show_settings = false;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (!logged_in) {
            // Authorization window
            ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_Always);
            ImGui::SetNextWindowPos(ImVec2(440, 210), ImGuiCond_Always);
            ImGui::Begin("Authorization", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);

            ImGui::Text("Email or Login:");
            ImGui::InputText("##email", login_email, sizeof(login_email));

            ImGui::Text("Password:");
            ImGui::InputText("##password", login_password, sizeof(login_password), ImGuiInputTextFlags_Password);

            if (ImGui::Button("Login")) {
                if (authenticate(users, std::string(login_email), std::string(login_password), current_user)) {
                    logged_in = true;
                    error_message.clear();
                    // Load user data by either email or login
                    for (const auto& user : users) {
                        if (user.email == std::string(login_email) || user.login == std::string(login_email)) {
                            strcpy(profile_name, user.name.c_str());
                            strcpy(profile_birthdate, user.birth.c_str());
                            profile_email = user.email;
                            break;
                        }
                    }
                } else {
                    error_message = "Invalid email or password. Register on the website.";
                    memset(login_email, 0, sizeof(login_email));
                    memset(login_password, 0, sizeof(login_password));
                }
            }

            if (!error_message.empty()) {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "%s", error_message.c_str());
            }

            ImGui::Text("Register on the website:");
            ImGui::SameLine();
            if (ImGui::TextLinkOpenURL("http://localhost:8080/register")) {
                // Link clicked, but TextLinkOpenURL handles opening
            }

            ImGui::End();
        } else {
            // Main interface
            ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
            ImGui::SetNextWindowSize(io.DisplaySize, ImGuiCond_Always);
            ImGui::Begin("Electronic Diary", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);

            // Header section
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.94f, 0.84f, 0.68f, 1.00f));
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.80f, 0.56f, 0.30f, 1.00f));
            ImGui::BeginChild("HeaderBar", ImVec2(0, 80), true, ImGuiWindowFlags_NoScrollbar);
            ImGui::Text("Electronic Diary");
            ImGui::SameLine();
            ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - 110);
            if (ImGui::Button("Profile", ImVec2(100, 28))) {
                show_profile_window = !show_profile_window;
            }
            ImGui::EndChild();
            ImGui::PopStyleColor(2);

            ImGui::Separator();

            // Navigation row
            ImGui::BeginChild("NavRow", ImVec2(0, 56), false, ImGuiWindowFlags_NoScrollbar);
            float navWidth = ImGui::GetContentRegionAvail().x;
            float buttonWidth = 120.0f;
            float spacing = 10.0f;
            float totalWidth = sections.size() * buttonWidth + (sections.size() - 1) * spacing;
            float offset = (navWidth > totalWidth) ? (navWidth - totalWidth) * 0.5f : 0.0f;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);
            for (int i = 0; i < sections.size(); ++i) {
                if (ImGui::Button(sections[i].c_str(), ImVec2(buttonWidth, 30))) {
                    current_section = i;
                }
                if (i + 1 < sections.size()) ImGui::SameLine();
            }
            ImGui::EndChild();

            ImGui::Dummy(ImVec2(0, 8));

            // Content panels
            ImVec2 avail = ImGui::GetContentRegionAvail();
            bool vertical = avail.x < 1000.0f;
            float leftWidth = vertical ? avail.x : avail.x * 0.75f;
            float rightWidth = vertical ? avail.x : avail.x - leftWidth - 12.0f;

            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.97f, 0.92f, 0.84f, 1.00f));
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.73f, 0.47f, 0.20f, 1.00f));
            if (vertical) {
                ImGui::BeginChild("LeftPanel", ImVec2(0, 320), true);
            } else {
                ImGui::BeginChild("LeftPanel", ImVec2(leftWidth, 0), true);
            }
            if (current_section == 0) {
                ImGui::Text("Grades:");
                ImGui::Separator();
                for (const auto& subject : grades) {
                    ImGui::Text("%s:", subject.first.c_str());
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0.16f, 0.12f, 0.07f, 1.00f), "%s", [&subject](){
                        std::string s;
                        for (int grade : subject.second) {
                            if (!s.empty()) s += ", ";
                            s += std::to_string(grade);
                        }
                        return s;
                    }().c_str());
                }
            } else if (current_section == 1) {
                ImGui::Text("Schedule: (will be implemented)");
                ImGui::TextWrapped("The schedule area will display classes and dates once the feature is ready.");
            } else if (current_section == 2) {
                ImGui::Text("Tasks: (will be implemented)");
                ImGui::TextWrapped("Task tracking will appear here in the next update.");
            }
            ImGui::EndChild();

            if (!vertical) ImGui::SameLine();

            ImGui::BeginChild("RightPanel", ImVec2(rightWidth, 0), true);
            ImGui::Text("Ad");
            ImGui::Separator();
            ImGui::TextWrapped("This section can show school news, reminders, or tips to keep your grades high.");
            ImGui::Dummy(ImVec2(0, 8));
            ImGui::Text("Student Profile");
            ImGui::Separator();
            ImGui::Text("Name: %s", profile_name);
            ImGui::Text("Email: %s", profile_email.c_str());
            ImGui::Text("Birthday: %s", profile_birthdate);
            ImGui::EndChild();
            ImGui::PopStyleColor(2);

            // Profile popup and confirmation dialogs
            if (show_profile_window) {
                ImGui::SetNextWindowSize(ImVec2(320, 180));
                ImGui::Begin("Profile", &show_profile_window);
                if (ImGui::Button("Profile Settings")) {
                    show_settings = true;
                    show_profile_window = false;
                }
                if (ImGui::Button("Logout")) {
                    show_logout_confirm = true;
                }
                ImGui::End();
            }

            if (show_logout_confirm) {
                ImGui::SetNextWindowSize(ImVec2(260, 110));
                ImGui::Begin("Logout Confirmation", &show_logout_confirm);
                ImGui::Text("Are you sure you want to logout?");
                if (ImGui::Button("Yes")) {
                    logged_in = false;
                    current_user.clear();
                    memset(login_email, 0, sizeof(login_email));
                    memset(login_password, 0, sizeof(login_password));
                    show_logout_confirm = false;
                    show_profile_window = false;
                }
                ImGui::SameLine();
                if (ImGui::Button("No")) {
                    show_logout_confirm = false;
                }
                ImGui::End();
            }

            if (show_settings) {
                ImGui::SetNextWindowSize(ImVec2(420, 320));
                ImGui::Begin("Profile Settings", &show_settings);
                ImGui::Text("Name:");
                ImGui::InputText("##name", profile_name, sizeof(profile_name));
                ImGui::Text("Date of Birth:");
                ImGui::InputText("##birthdate", profile_birthdate, sizeof(profile_birthdate));
                ImGui::Text("Email: %s", profile_email.c_str());
                ImGui::Text("(Email cannot be changed)");
                if (ImGui::Button("Save")) {
                    // Update user data
                    for (auto& user : users) {
                        if (user.email == profile_email) {
                            user.name = std::string(profile_name);
                            user.birth = std::string(profile_birthdate);
                            break;
                        }
                    }
                    saveUsers("/home/trev/Dev/College/sputidiyu/db.txt", users); // Save to parent directory
                    show_settings = false;
                }
                ImGui::End();
            }

            ImGui::End();
        }

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}