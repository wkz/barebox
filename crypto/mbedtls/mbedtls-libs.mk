mbedtls-libs-y := libtfpsacrypto.a
mbedtls-libs-$(CONFIG_MBEDTLS_NEEDS_LIB_TLS) += libmbedtls.a
mbedtls-libs-$(CONFIG_MBEDTLS_NEEDS_LIB_X509) += libmbedx509.a
mbedtls-libs += $(addprefix build/library/,$(mbedtls-libs-y))
