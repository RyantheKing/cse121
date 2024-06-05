# Embed the server root certificate into the final binary
#
# (If this was a component, we would set COMPONENT_EMBED_TXTFILES here.)
idf_component_register(SRCS "lab7_2.c" "time_sync.c" "temp.c"
                    INCLUDE_DIRS "."
                    EMBED_TXTFILES server_root_cert.pem)
