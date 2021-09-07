obj-$(CONFIG_CRYPTO_DEV_PADLOCK_AES) += padlock-aes.o
obj-$(CONFIG_CRYPTO_DEV_PADLOCK_SHA) += padlock-sha.o
obj-$(CONFIG_CRYPTO_DEV_GEODE) += geode-aes.o
obj-$(CONFIG_CRYPTO_DEV_HIFN_795X) += hifn_795x.o
obj-$(CONFIG_CRYPTO_DEV_MV_CESA) += mv_cesa.o
ifeq ($(CONFIG_SYNO_QORIQ),n)
obj-$(CONFIG_CRYPTO_DEV_TALITOS) += talitos_qoriq.o
else
obj-$(CONFIG_CRYPTO_DEV_TALITOS) += talitos.o
endif
obj-$(CONFIG_CRYPTO_DEV_IXP4XX) += ixp4xx_crypto.o
obj-$(CONFIG_CRYPTO_DEV_PPC4XX) += amcc/

ifeq ($(CONFIG_SYNO_QORIQ),y)
obj-$(CONFIG_CRYPTO_DEV_EXTENS_T23X) += t23x/t23xrm/
obj-$(CONFIG_CRYPTO_DEV_EXTENS_T23X_RMTEST) += t23x/t23xtest/
obj-$(CONFIG_CRYPTO_DEV_EXTENS_T23X_SEC2) += t23x/t23xsec2/
endif
