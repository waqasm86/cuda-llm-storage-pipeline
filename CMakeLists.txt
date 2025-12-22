cmake_minimum_required(VERSION 3.22)
project(cuda_llm_storage_pipeline LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(SLP_ENABLE_SANITIZERS "Enable ASAN/UBSAN" OFF)

# ---- warnings ----
add_compile_options(
  -Wall
  -Wextra
  -Wpedantic
  -Wshadow
  -Wconversion
)

if (SLP_ENABLE_SANITIZERS)
  add_compile_options(-fsanitize=address,undefined)
  add_link_options(-fsanitize=address,undefined)
endif()

# ---- dependencies ----
find_package(CURL REQUIRED)

# ---- library ----
add_library(slp_core
  src/http_client.cpp
  src/sha256.cpp

  src/seaweed/lookup.cpp
  src/seaweed/assign.cpp
  src/seaweed/file_upload.cpp
  src/seaweed/file_download.cpp
  src/seaweed/filer.cpp

  src/artifact/manifest.cpp
  src/artifact/registry.cpp
  src/artifact/paths.cpp

  src/pipeline/model_store.cpp
  src/pipeline/prompt_store.cpp
  src/pipeline/result_store.cpp
  src/pipeline/run_id.cpp
)

target_include_directories(slp_core
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(slp_core
  PUBLIC
    CURL::libcurl
)

# ---- executables ----
function(add_slp_app name)
  add_executable(${name} apps/${name}.cpp)
  target_link_libraries(${name} PRIVATE slp_core)
endfunction()

add_slp_app(slp_put_model)
add_slp_app(slp_get_model)
add_slp_app(slp_put_prompts)
add_slp_app(slp_run_infer)
add_slp_app(slp_bench_storage)

# Direct llama-server client (no SeaweedFS dependency)
add_executable(slp_llama_client apps/slp_llama_client.cpp)
target_link_libraries(slp_llama_client PRIVATE CURL::libcurl)

# Batch inference with results storage
add_executable(slp_llama_batch apps/slp_llama_batch.cpp)
target_link_libraries(slp_llama_batch PRIVATE CURL::libcurl)
