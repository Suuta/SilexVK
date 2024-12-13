#pragma once

#include "Core/Core.h"
#include <imgui/imgui.h>


namespace Silex
{
    class ConsoleLogger
    {
    public:

        static ConsoleLogger& Get();

        void Log(LogLevel level, const std::string& message)
        {
            std::lock_guard<std::mutex> lock(mutex);

            // 一定サイズなら、先頭要素を消す
            if (logEntries.size() > maxLogEntory)
                logEntries.pop_front();

            LogEntry entry;
            entry.level = level;
            entry.text  = message;

            logEntries.emplace_back(entry);
        }

        void Clear()
        {
            std::lock_guard<std::mutex> lock(mutex);
            logEntries.clear();
        }

        void LogData()
        {
            ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

            for (auto& entry : logEntries)
            {
                ImVec4 color;
                
                switch(entry.level)
                {
                    case LogLevel::Fatal : color = ImVec4(0.8f, 0.0f, 0.8f, 1.0f); break;
                    case LogLevel::Error : color = ImVec4(0.8f, 0.2f, 0.2f, 1.0f); break;
                    case LogLevel::Warn  : color = ImVec4(0.8f, 0.6f, 0.0f, 1.0f); break;
                    case LogLevel::Info  : color = ImVec4(0.0f, 0.6f, 0.0f, 1.0f); break;
                    case LogLevel::Trace : color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f); break;
                    case LogLevel::Debug : color = ImVec4(0.0f, 0.5f, 0.8f, 1.0f); break;
                
                    default: color = ImVec4(1, 1, 1, 1); break;
                }

                ImGui::PushStyleColor(ImGuiCol_Text, color);
                ImGui::Text("%s", entry.text.c_str());
                ImGui::PopStyleColor();
            }

            ImGui::EndChild();
        }

    private:

        struct LogEntry
        {
            std::string text;
            LogLevel    level;
        };

    private:

        const uint64 maxLogEntory = 1024;

        std::deque<LogEntry> logEntries;
        std::mutex           mutex;
    };
}

