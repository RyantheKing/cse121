# Embed the server root certificate into the final binary
#
# (If this was a component, we would set COMPONENT_EMBED_TXTFILES here.)
idf_component_register(SRCS "lab7_3.c" "time_sync.c" "temp.c" "get_http.c" "get_https.c" "post.c"
                    INCLUDE_DIRS "."
                    EMBED_TXTFILES server_root_cert.pem)
