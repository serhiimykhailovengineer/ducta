// Configuration of Ducta for this program (see ducta/Core/Config.hpp and ducta/Core/Capacity.hpp)
#pragma once

// PlatformIO selects the embedded backend (ETL) by default; spelled out here for clarity
#define DUCTA_USE_EMBEDDED_BACKEND

// Small capacities and no execution trace...
#define DUCTA_CAPACITY_PROFILE_MCU

// ...with the ones this program needs
#define DUCTA_MAX_NODES 8
#define DUCTA_MAX_NODE_TYPES 8
#define DUCTA_NODE_STORAGE_BYTES 512
