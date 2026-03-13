aux_source_directory(. SRC)

aux_source_directory(buffer BUFFER)
aux_source_directory(epoller EPOLLER)
aux_source_directory(http HTTP)
aux_source_directory(log LOG)
aux_source_directory(pool POOL)
aux_source_directory(server SERVER)
aux_source_directory(timer TIMER)

add_executable(webserver
    ${SRC}
    ${BUFFER}
    ${EPOLLER}
    ${HTTP}
    ${LOG}
    ${POOL}
    ${SERVER}
    ${TIMER}
)

target_include_directories(webserver PRIVATE
    ${PROJECT_SOURCE_DIR}/code
)

target_link_libraries(webserver
    pthread
    mysqlclient
)