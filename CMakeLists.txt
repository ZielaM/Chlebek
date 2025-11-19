cmake_minimum_required(VERSION 3.16)
project(SiatkaGlutenowa)

set(CMAKE_CXX_STANDARD 17)

# Ustawiamy ścieżkę do vendora
set(VENDOR_DIR "${CMAKE_SOURCE_DIR}/vendor")

# ==========================================
# 1. BIBLIOTEKI "HEADER-ONLY" I ŹRÓDŁOWE
# (Te ładujemy zawsze z vendora, dla spójności)
# ==========================================

# --- GLAD ---
include_directories(${VENDOR_DIR}/glad/include)
set(GLAD_SOURCE "${VENDOR_DIR}/glad/src/glad.c")

# --- ImGui ---
include_directories(${VENDOR_DIR}/imgui)
include_directories(${VENDOR_DIR}/imgui/backends)
# Zbieramy pliki .cpp ImGui
file(GLOB IMGUI_SOURCES 
    "${VENDOR_DIR}/imgui/*.cpp" 
    "${VENDOR_DIR}/imgui/backends/imgui_impl_glfw.cpp" 
    "${VENDOR_DIR}/imgui/backends/imgui_impl_opengl3.cpp"
)

# --- GLM ---
# Ładujemy z vendora, żeby VS na Windowsie to widziało
include_directories(${VENDOR_DIR}/glm)

# ==========================================
# 2. BIBLIOTEKA GLFW (Rozwidlenie Systemowe)
# ==========================================

if(WIN32)
    # WINDOWS: Używamy plików, które właśnie pobrałeś do vendora
    message(STATUS "Wykryto Windows. Używam lokalnego GLFW z folderu vendor.")
    
    set(GLFW_INCLUDE_DIR "${VENDOR_DIR}/glfw/include")
    set(GLFW_LIB_DIR "${VENDOR_DIR}/glfw/lib-vc2022") # Sprawdź czy folder tak się nazywa!
    
    include_directories(${GLFW_INCLUDE_DIR})
    link_directories(${GLFW_LIB_DIR})
else()
    # LINUX: Używamy biblioteki systemowej (z apt-get)
    message(STATUS "Wykryto Linux. Szukam GLFW w systemie.")
    
    find_package(glfw3 REQUIRED)
endif()

# ==========================================
# 3. BUDOWANIE APLIKACJI
# ==========================================

# Znajdź wszystkie Twoje pliki .cpp w src
file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(${PROJECT_NAME} ${SOURCES} ${GLAD_SOURCE} ${IMGUI_SOURCES})

# ==========================================
# 4. LINKOWANIE (Łączenie z bibliotekami)
# ==========================================

if(WIN32)
    # Linkujemy na Windowsie
    target_link_libraries(${PROJECT_NAME} glfw3 opengl32 gdi32 user32 shell32)
    
    # AUTOMATYCZNE KOPIOWANIE DLL (Żebyś nie musiał tego robić ręcznie)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${VENDOR_DIR}/glfw/lib-vc2022/glfw3.dll"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
else()
    # Linkujemy na Linuxie
    target_link_libraries(${PROJECT_NAME} glfw GL dl)
endif()