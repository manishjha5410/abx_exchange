To run the code: -
g++ -std=c++17 -Wall -Ipath_to_boost_include -Lpath_to_boost_lib app.cpp -o main.exe -lboost_json -lboost_system -lws2_32 && .\main.exe

Eg: - (g++ -std=c++17 -Wall -I"D:\Program Files\MinGW\include" -L"D:\Program Files\MinGW\lib" app.cpp -o main.exe -lboost_json -lboost_system -lws2_32 && .\main.exe)