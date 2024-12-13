
#pragma once

#ifdef SL_PLATFORM_WINDOWS
#include <Windows.h>
#endif

#include <memory>
#include <vector>
#include <string>
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <thread>

#include "Core/Memory.h"
#include "Core/Hash.h"
#include "Core/Random.h"
#include "Core/Object.h"
#include "Core/Delegate.h"
#include "Core/Logger.h"
#include "Core/Event.h"
