#include <stdio.h>
#include <stdlib.h>
#include "string.h"
#include "stdint.h"
#include "inv_com.h"
#include "cat1.h"
#include "tianhe.h"

#include "asw_store.h"

#define ALIYUN_CA_PEM "-----BEGIN CERTIFICATE-----\n"                                      \
                      "MIIDdTCCAl2gAwIBAgILBAAAAAABFUtaw5QwDQYJKoZIhvcNAQEFBQAwVzELMAkG\n" \
                      "A1UEBhMCQkUxGTAXBgNVBAoTEEdsb2JhbFNpZ24gbnYtc2ExEDAOBgNVBAsTB1Jv\n" \
                      "b3QgQ0ExGzAZBgNVBAMTEkdsb2JhbFNpZ24gUm9vdCBDQTAeFw05ODA5MDExMjAw\n" \
                      "MDBaFw0yODAxMjgxMjAwMDBaMFcxCzAJBgNVBAYTAkJFMRkwFwYDVQQKExBHbG9i\n" \
                      "YWxTaWduIG52LXNhMRAwDgYDVQQLEwdSb290IENBMRswGQYDVQQDExJHbG9iYWxT\n" \
                      "aWduIFJvb3QgQ0EwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDaDuaZ\n" \
                      "jc6j40+Kfvvxi4Mla+pIH/EqsLmVEQS98GPR4mdmzxzdzxtIK+6NiY6arymAZavp\n" \
                      "xy0Sy6scTHAHoT0KMM0VjU/43dSMUBUc71DuxC73/OlS8pF94G3VNTCOXkNz8kHp\n" \
                      "1Wrjsok6Vjk4bwY8iGlbKk3Fp1S4bInMm/k8yuX9ifUSPJJ4ltbcdG6TRGHRjcdG\n" \
                      "snUOhugZitVtbNV4FpWi6cgKOOvyJBNPc1STE4U6G7weNLWLBYy5d4ux2x8gkasJ\n" \
                      "U26Qzns3dLlwR5EiUWMWea6xrkEmCMgZK9FGqkjWZCrXgzT/LCrBbBlDSgeF59N8\n" \
                      "9iFo7+ryUp9/k5DPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNVHRMBAf8E\n" \
                      "BTADAQH/MB0GA1UdDgQWBBRge2YaRQ2XyolQL30EzTSo//z9SzANBgkqhkiG9w0B\n" \
                      "AQUFAAOCAQEA1nPnfE920I2/7LqivjTFKDK1fPxsnCwrvQmeU79rXqoRSLblCKOz\n" \
                      "yj1hTdNGCbM+w6DjY1Ub8rrvrTnhQ7k4o+YviiY776BQVvnGCv04zcQLcFGUl5gE\n" \
                      "38NflNUVyRRBnMRddWQVDf9VMOyGj/8N7yy5Y0b2qvzfvGn9LhJIZJrglfCm7ymP\n" \
                      "AbEVtQwdpf5pLGkkeB6zpxxxYu7KyJesF12KwvhHhm4qxFYxldBniYUr+WymXUad\n" \
                      "DKqC5JlR3XC321Y9YeRq4VzW9v493kHMB65jUr9TU/Qr6cf9tveCX4XSQRjbgbME\n" \
                      "HMUfpIBvFSDJ3gyICh3WZlXi/EjJKSZp4A==\n"                             \
                      "-----END CERTIFICATE-----"

//for liaoNingGrid
// #define SERVER_CA_PEM "-----BEGIN CERTIFICATE-----\n"                                      \
//                       "MIICuDCCAiGgAwIBAgIJAP+L+/yzpCHgMA0GCSqGSIb3DQEBBQUAMHQxCzAJBgNV\n" \
//                       "BAYTAnpoMQswCQYDVQQIDAJkbDELMAkGA1UEBwwCZGwxDTALBgNVBAoMBHNvZnQx\n" \
//                       "CzAJBgNVBAsMAmRsMREwDwYDVQQDDAhjanNlcnZlcjEcMBoGCSqGSIb3DQEJARYN\n" \
//                       "Y2pAc2VydmVyLmNvbTAgFw0xNjA4MzEwOTE0MjhaGA8yMTE2MDgwNzA5MTQyOFow\n" \
//                       "dDELMAkGA1UEBhMCemgxCzAJBgNVBAgMAmRsMQswCQYDVQQHDAJkbDENMAsGA1UE\n" \
//                       "CgwEc29mdDELMAkGA1UECwwCZGwxETAPBgNVBAMMCGNqc2VydmVyMRwwGgYJKoZI\n" \
//                       "hvcNAQkBFg1jakBzZXJ2ZXIuY29tMIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKB\n" \
//                       "gQDF8M2VsIjtwOOn2mNEqJ+yL7Lz2SeRVfnx/mmYs5fqYN//mz/LQtX6DLhuUIg3\n" \
//                       "nehDa7sX1VQeFd7YuVCv7aKoUfHLllfy5MWq5leM+F2UOkH1IF6BSl+PRxgIwAEQ\n" \
//                       "X2M69VhSCQva6p/dZs0pdn1GW2bGrp1WjNdwfH5qor+zLwIDAQABo1AwTjAdBgNV\n" \
//                       "HQ4EFgQUxrYkWFqCIsmasiWXptWzhcDJABkwHwYDVR0jBBgwFoAUxrYkWFqCIsma\n" \
//                       "siWXptWzhcDJABkwDAYDVR0TBAUwAwEB/zANBgkqhkiG9w0BAQUFAAOBgQAurlSk\n" \
//                       "YBsorMV+4zPOH1unbdph5K1W0soUtLNfejWUWDEJQRoYA2UxPPbVYkzdEsTsbgnt\n" \
//                       "ZeHF6gk1V9VmDhmVKFAy141iYlLjIJxNHVnhGZDot06oPsF+gZ1ymAzX6RB+I9GD\n" \
//                       "N8BeBxfUcpriXykKg5MraSlMsjyeD1XhwZfJ/w==\n"                         \
//                       "-----END CERTIFICATE-----"

#define SERVER_CA_PEM "-----BEGIN CERTIFICATE-----\n"                                      \
                      "MIIE8zCCA1ugAwIBAgIJAMHgyL6Bw9f0MA0GCSqGSIb3DQEBCwUAMIGOMQswCQYD\n" \
                      "VQQGEwJDTjEQMA4GA1UECAwHSmlhbmdzdTESMBAGA1UEBwwJQ2hhbmd6aG91MRIw\n" \
                      "EAYDVQQKDAl0cmluYWJsdWUxCzAJBgNVBAsMAklUMRgwFgYDVQQDDA8qLnRyaW5h\n" \
                      "Ymx1ZS5jb20xHjAcBgkqhkiG9w0BCQEWDypAdHJpbmFibHVlLmNvbTAgFw0yMjA3\n" \
                      "MDQwNjI1MTVaGA8yMDY5MDgwMTA2MjUxNVowgY4xCzAJBgNVBAYTAkNOMRAwDgYD\n" \
                      "VQQIDAdKaWFuZ3N1MRIwEAYDVQQHDAlDaGFuZ3pob3UxEjAQBgNVBAoMCXRyaW5h\n" \
                      "Ymx1ZTELMAkGA1UECwwCSVQxGDAWBgNVBAMMDyoudHJpbmFibHVlLmNvbTEeMBwG\n" \
                      "CSqGSIb3DQEJARYPKkB0cmluYWJsdWUuY29tMIIBojANBgkqhkiG9w0BAQEFAAOC\n" \
                      "AY8AMIIBigKCAYEAo8qN/NrK06q3PlbkfPGleZY1HjfMNZIYhRC2EVz77/pa2/72\n" \
                      "YE2pJ66d33SoYCClhH5Ipm/2M/xaTpTUTEQKefsuDufyM8avsWPYDzmYOadR+q8f\n" \
                      "+ws2Ln7UjGJSWEhm8Ni1Jhj5lv5vQb3ZUzQtm8TJfBqpmAeHA87zSK+5AfUns/48\n" \
                      "yicB3MX4y5HtxTrMauMiMuawU2Icg6NvPpDVYRMfvetc9O8Xde/iij4ObGHV3Mel\n" \
                      "I155i1XwlWpq5Zvh3MBY/zz7cRWpFtSlm64215HfE7CKSeekkF4rGOybr7z/4z5d\n" \
                      "HzgjcAM1hT0ixFqU/jnVJK7nRw9ZfLHbSj8dJxdVHqk2Kc8/bBOWNMMeqSSeJ1YF\n" \
                      "A+EjZkLFjoKGFKiYdXXUfaHp8cQ64966qxTuOc4UzvmvEkTKZ2scfBcqrzOLvMwP\n" \
                      "nzGm+e8b0vWR/+ihanPY8SANrXsGYuwXZickqGy7ZCE8PgFv5BdoqvtO6roUtdUh\n" \
                      "sKVJ9UFw7OAzqCEVAgMBAAGjUDBOMB0GA1UdDgQWBBT8pVSdlH2sC4ZQ3iiW4dZc\n" \
                      "WfwocDAfBgNVHSMEGDAWgBT8pVSdlH2sC4ZQ3iiW4dZcWfwocDAMBgNVHRMEBTAD\n" \
                      "AQH/MA0GCSqGSIb3DQEBCwUAA4IBgQB4sxPKmsYkRg4SGCxeuYxuhe04Ip4Grmxk\n" \
                      "gNaHvXDIbrJlU+7IMQ14XHANEkz4h3r7J7JbCmMzvRSxY/dn3YK7KwNvdtFCi+1P\n" \
                      "1Uu2toZJZBgkC3NNEk3980egWqRZ4bgD1zXkc6yMJw+Cak/ZE/Ja8JzuTJRuXMVO\n" \
                      "V+ZBzPELnM5zy1YbvritI9ROt3en9XlV6YhL+kD3VPJxOkqjoDIIkTndoM0baJv3\n" \
                      "02If0INxdx14zdWySxnMJqMtDzoWlON2LbvmVTf48zAMBSk4UeXSaTGbiv2llMeQ\n" \
                      "qcIoPJB6OLX7jvWHiZ7XDOr1sx+vW9mypvtGI2DLeZOVl9rfC5AR9wRcfBuQRGeg\n" \
                      "PeFSFPR/U8VARM28TTirC3lAjbJHvumKgrDCd2jiozmYaESjMG8OKJveFma4kCUG\n" \
                      "YYuV91C4j+KEQJpUK8P2HHiwuQrc8Mqi4dvf8IGSknbOeyKCTGeO0TDw+9hdSU93\n" \
                      "OJsepjnkyv5z6T0WDEoYUc7Fr0kDWEU=\n"                                 \
                      "-----END CERTIFICATE-----\n"

#define CLIENT_CRT_PEM "-----BEGIN CERTIFICATE-----\n"                                      \
                       "MIIExDCCAyygAwIBAgIJANWpF9MubdgPMA0GCSqGSIb3DQEBCwUAMIGOMQswCQYD\n" \
                       "VQQGEwJDTjEQMA4GA1UECAwHSmlhbmdzdTESMBAGA1UEBwwJQ2hhbmd6aG91MRIw\n" \
                       "EAYDVQQKDAl0cmluYWJsdWUxCzAJBgNVBAsMAklUMRgwFgYDVQQDDA8qLnRyaW5h\n" \
                       "Ymx1ZS5jb20xHjAcBgkqhkiG9w0BCQEWDypAdHJpbmFibHVlLmNvbTAgFw0yMjA3\n" \
                       "MDYwMTM5NDVaGA8yMDY5MDcxNjAxMzk0NVowgZUxCzAJBgNVBAYTAkNOMRAwDgYD\n" \
                       "VQQIDAdKaWFuZ3N1MQ8wDQYDVQQHDAZTdVpob3UxDzANBgNVBAoMBkFpU1dFSTEP\n" \
                       "MA0GA1UECwwGQWlTV0VJMRUwEwYDVQQDDAwqLkFpU1dFSS5jb20xKjAoBgkqhkiG\n" \
                       "9w0BCQEWG1NhbGVzLkNoaW5hQGFpc3dlaS10ZWNoLmNvbTCCAaIwDQYJKoZIhvcN\n" \
                       "AQEBBQADggGPADCCAYoCggGBANbCc7Q2dUb7U8gvTNO8Pk4ZFkF3IrDpGOQG0kN6\n" \
                       "y0p83f5vpvvymY9/FCslMEgvlW3CTns50Zye1wDXIZUyWOXBKhmdE5DKrXevHYMX\n" \
                       "tsFQnrqCua/cuCRVAQP/53kAblhimdN6RBRv6WgoEtLf1yJZvqDtN1ONgI6Y1qrl\n" \
                       "YIi8rvYEXDNs9Z69qjHWLq+n2AGQxZDnGt2aXNBu2QZMeGdeOTJ2STvteJvlZnsy\n" \
                       "+s4RNFu+bzjUao5G5fg6U7+YNLix+v+vEbChFh5a6wF6mq2ZHiOQnJZWsQ1s2npT\n" \
                       "p7gj0IBwVC357RhUUTS47j0hL6x2ShZ3QMIW2d8Gy31ntHqthC6D+gX6HI8z1yFy\n" \
                       "LKsV5P0Cf1CpWMhckmRetrwIAn8uOrpjqrBiuCJnfDSUpFPp2FOF3GX34ysAQtDE\n" \
                       "D/jrI1rZ7g4mVGvum1l67v3QCWdUbPrWKRpAKei0qIUe1sJyDb95z6flRhDSwSrM\n" \
                       "2TgqEKU4A4W2B6sco9lk92iVIwIDAQABoxowGDAJBgNVHRMEAjAAMAsGA1UdDwQE\n" \
                       "AwIF4DANBgkqhkiG9w0BAQsFAAOCAYEAZ4REqF6Obm2EuqLKO5r+GgMBxVMLTNT1\n" \
                       "p8tFfBmajIdVaP9guFERTX6azr/9fFnPJnAGFOB5BOXFR7tqBZeAbdx4xWCCpaIB\n" \
                       "4JA3208XD/tSFxeBRzRv7zpHkiDu4cE3wd2kS7pwQV8geeiQgdMbqypfHt0NCU1N\n" \
                       "MyEHr7+vYml4XgsIVUpuusozBtua0AA5fWHfWwyQG6olIYHA4nEFQ+XlHFT2D8GB\n" \
                       "puCJ5Dqcnh6JW+DIwQtyV90vLzX9fvfmqzHzAFjiuj4ryQCoEov46ISwYzmbKL/W\n" \
                       "+tkB14m9Tx8BHScL2OK7TlhDGtzx8GVzxmiaFKuFqTlAZbC+hkaS0ichRt/AsHQa\n" \
                       "hGFlfy07ax9K4ie/P3iLhbKkiMGtM+StEfkh7G29XEle+Hpcj61Wp/JSBbaLvue8\n" \
                       "tuuEE0JOgVKLNgaLrrIp7EGJ9c9egyUn543qcaXgfLoWQagF+QZClApe5U0wxLuQ\n" \
                       "syTpGFxfdIvChPQ92jZMSzyEt03sSnFo\n"                                 \
                       "-----END CERTIFICATE-----\n"

#define CLIENT_KEY_PEM "-----BEGIN RSA PRIVATE KEY-----\n"                                  \
                       "MIIG5AIBAAKCAYEA1sJztDZ1RvtTyC9M07w+ThkWQXcisOkY5AbSQ3rLSnzd/m+m\n" \
                       "+/KZj38UKyUwSC+VbcJOeznRnJ7XANchlTJY5cEqGZ0TkMqtd68dgxe2wVCeuoK5\n" \
                       "r9y4JFUBA//neQBuWGKZ03pEFG/paCgS0t/XIlm+oO03U42AjpjWquVgiLyu9gRc\n" \
                       "M2z1nr2qMdYur6fYAZDFkOca3Zpc0G7ZBkx4Z145MnZJO+14m+VmezL6zhE0W75v\n" \
                       "ONRqjkbl+DpTv5g0uLH6/68RsKEWHlrrAXqarZkeI5CcllaxDWzaelOnuCPQgHBU\n" \
                       "LfntGFRRNLjuPSEvrHZKFndAwhbZ3wbLfWe0eq2ELoP6BfocjzPXIXIsqxXk/QJ/\n" \
                       "UKlYyFySZF62vAgCfy46umOqsGK4Imd8NJSkU+nYU4XcZffjKwBC0MQP+OsjWtnu\n" \
                       "DiZUa+6bWXru/dAJZ1Rs+tYpGkAp6LSohR7WwnINv3nPp+VGENLBKszZOCoQpTgD\n" \
                       "hbYHqxyj2WT3aJUjAgMBAAECggGBALAq0r8B7TJM+G0+X8dQo8tsyNTq5Yo5rDFh\n" \
                       "ZxnzoM95nqEY9eG3IECV/fVmjDSq0+k1eyuaQlg39Ca8UtAQfNv7mI1qTKJ5n9KN\n" \
                       "06e1zDTH7W0Rz0Bzzpn/INYnFbosoFfgik7v/OjG5LCMLuTMua1z8OwRq1DewpY2\n" \
                       "yeFmR8ni/aLR8NA+kT2mV/aJu8Jb7NVb7LTw3xjphzlztJN2J06j/EdowKpoIoe/\n" \
                       "plFpgFOJyWl4zcPQe5g6kGsPbaO9NgGXlcoN2dhDyo0VS8zJg9j+SCNfPXP3mBlb\n" \
                       "/GiYo9OwSM+CgxSZ5cYYYJDxV6D6RoNBXt09U5Yizm6dOaiIoHjaETwPVAmcg2ov\n" \
                       "hUWeZ7/xJ52AIl2vrEEXH7m3MjDAisfLI5FmJcIki5WVPlIvYM1WfyxhkmFVCCvD\n" \
                       "WICchL0Zxw0AUBIdDZAqbiQmt+5CuSp5euf6Se7i7gz6voK+iAYQP+ZeCo+WFnr7\n" \
                       "x77iKAI0sVYLgHV0ppCmCGiJMrWwAQKBwQDs1+kl1Ig5uuKPt07a6xHF/Zr88j1N\n" \
                       "oeJvpl+zVlJnxcCD7FnDpZC5fRWx1fvbRf1oW+G2erUJOCvX2xU17ckZ1eDPYIs8\n" \
                       "VmCVu+rafvGKyuh68XuWfpq4aE/yU6kKeBoWhqbGeJnXP0e741RjbSe0eW8XxtpZ\n" \
                       "9Qu7syH3yD0btFaQlrn++1q5qBjsGBN90wUsPNSv2KzjI1x5EftGXEnB8L1vDebT\n" \
                       "WkaEue/WKRE/tQn5LXjXWjxI3orAVyJqQ8UCgcEA6CFFzhpds7XiGvuRUlhLLbFM\n" \
                       "IioaAvfMGPSzIu1tLdVktg/GfupUmV0kwxsicohq9qf1o4JolPQkzVzkRVUAGFXC\n" \
                       "9iUvR0ZqRDKEGazrcFOi1pnacT4zKdfCq4i2CWWRNQujXBm8isP7tGEDXNKFK/nM\n" \
                       "knKExqR49G1pqrZP/8/FSog+8WQeqXm3sPGoMza7yXrE7wDnmaRIovIqNaGV0XTK\n" \
                       "Q3qcHiPOtD0SRTO/kZ2eZG/rlz+feQorUG/V6bvHAoHAPLWvlrne3WVxM8OaG/WL\n" \
                       "jcPPGa2CV0b6wwHToCWtY9pF2csYy1TCPzm3+OjP8UoEhd6fgeX8R8u1OVR4IGW+\n" \
                       "WbgAM0gdCK9ffKI5th46DgirBPCnbFExmblrXMNjHkLg9Qs47sD1Nj6LRbck54OQ\n" \
                       "XOuPtXmMeOieRFPLjjcuqs5ULiXksV5+x/41vTcCsAsKVWeWYSjaUDK9Pm+EiGmZ\n" \
                       "onPyKuhWVbDbSKRcvPmz6S+fydaFFjaUI12gFWOAd9q9AoHBAIDUFn2wPFjElNwM\n" \
                       "yjY9MlwFjg9X9l+3Ttp61ACKbJGHYQF599vwCUukWga+RHv0IgkKZlV8xrYEWXhw\n" \
                       "GXAxIIBg+HbLZFXWSpvWvWHSikgo4rLfaFB5CMQsLqoskXrdl50s/FjdH7qT0lnl\n" \
                       "jYwyeh5R4KtWS0JDfa2KG57W18dNdF1NCEHIIwxDtSLKu2LQ+Z90N1+9zMuEVZu5\n" \
                       "cpuZNiCtBKQ2o/ZGQS3exwkJE0SpiYKMat/iO4tdjXRy3PF/eQKBwDFQFhyI7eTP\n" \
                       "SZhcDzGteLetNTV6+20Zu4saHB9KHY2O4yQqTte1Q/TCc2IWfZeZNnk3WD98C8aT\n" \
                       "q5XpJIHSMl+R2/lpwFJT910Hk5vIz12oQTgnQVcQArOn7lhynvVV92Cdvi+7DXx4\n" \
                       "3nDmX87Pz1oUk2GsKqDmyoVKCJIn7WmZmTjq/owoR0NletwFUnQs9u75BP58Y8vC\n" \
                       "8fySH07Hvq1iMprmyL7tXbxine779PclnvZcMu4HD0nuUokXtHP5Uw==\n"         \
                       "-----END RSA PRIVATE KEY-----\n"

#define FAST_LOG_EN 1

/** data call commands *******************************************************************************/
#define at "AT"
#define at_disable_echo "ATE0"
#define at_reboot "AT+CFUN=1,1"

#define at_cereg "AT+CEREG?"
#define at_cgreg "AT+CGREG?"

#define at_csq "AT+CSQ"
#define at_net_info "AT+COPS?" //CMCC, 4G/2G

#define at_set_apn "AT+QICSGP=1,1,\"%s\",\"\",\"\",1"
#define at_data_call "AT+QIACT=1"
#define at_data_call_clear "AT+QIDEACT=1"
#define at_check_data_call "AT+QIACT?"

#define at_qccid "AT+QCCID"
#define at_imei "AT+GSN"
#define at_imsi "AT+CIMI"

#define at_set_creg "AT+CREG=2"
#define at_creg "AT+CREG?"

/** tcp commands *******************************************************************************/
#define at_qiopen "AT+QIOPEN=1,0,\"TCP\",\"%s\",%d,0,0" // host, port
#define at_qistate "AT+QISTATE=1,0"
#define at_qisend "AT+QISEND=0,%d" // size
#define at_qird "AT+QIRD=0,1400"   // size
#define at_qiclose "AT+QICLOSE=0"

/** ssl commands *******************************************************************************/
//idx=0-5

//sslversion 0 ssl3.0
//sslversion 1 tls1.0
//sslversion 2 tls1.1
//sslversion 3 tls1.2
//sslversion 4 all

//ciphersuite 0XFFFF support all

//seclevel 0 无身份验证模式
//seclevel 1 进行服务器身份验证
//seclevel 2 如果远程服务器要求，则进行服务器和客户端身份验证

#define at_ssl_version "AT+QSSLCFG=\"sslversion\",0,4"                       //4:支持所有类型
#define at_ssl_cipher "AT+QSSLCFG=\"ciphersuite\",0,0XFFFF"                  //0XFFFF:支持所有类型
#define at_ssl_level "AT+QSSLCFG=\"seclevel\",0,2"                           //two way verify
#define at_ssl_ignore_valid "AT+QSSLCFG=\"ignoreinvalidcertsign\",%d,1"      //ssl idx
#define at_ssl_ignore_chain "AT+QSSLCFG=\"ignoremulticertchainverify\",%d,1" //ssl idx
#define at_ssl_crt "AT+QSSLCFG=\"cacert\",0,\"UFS:%s\""                      //.pem
#define at_ssl_client_crt "AT+QSSLCFG=\"clientcert\",0,\"UFS:%s\""           //.pem
#define at_ssl_client_key "AT+QSSLCFG=\"clientkey\",0,\"UFS:%s\""            //.pem=.key+pass_phrase

#define at_ssl_open "AT+QSSLOPEN=1,0,0,\"%s\",%d,0" //pdpId, sslId, clientId, ip port, cacheMode
#define at_ssl_state "AT+QSSLSTATE=0"
#define at_ssl_send "AT+QSSLSEND=0,%d" //msgLen
#define at_ssl_rd "AT+QSSLRECV=0,1400"
#define at_ssl_close "AT+QSSLCLOSE=0"

/** mqtt commands ************************************************************************************/
#define at_upload_ca_cert "AT+QFUPL=\"UFS:%s\",%d,5"
#define at_del_ca_cert "AT+QFDEL=\"UFS:%s\""
#define at_read_ca_cert "AT+QFDWL=\"UFS:%s\""
#define at_ls "AT+QFLST"

//server side
static const char *ssl_name_tab[2] = {"server.pem", "aliyun.pem"};
#define ssl_body_tab(x)          \
    ({                           \
        char *msg = NULL;        \
        if ((x) == 0)            \
        {                        \
            msg = SERVER_CA_PEM; \
        }                        \
        else if ((x) == 1)       \
        {                        \
            msg = ALIYUN_CA_PEM; \
        }                        \
        else                     \
        {                        \
            msg = "";            \
        }                        \
        msg;                     \
    })

//tcp client side
static const char *cli_name_tab[2] = {"cli_crt.pem", "cli_key.pem"};
#define cli_body_tab(x)           \
    ({                            \
        char *msg = NULL;         \
        if ((x) == 0)             \
        {                         \
            msg = CLIENT_CRT_PEM; \
        }                         \
        else if ((x) == 1)        \
        {                         \
            msg = CLIENT_KEY_PEM; \
        }                         \
        else                      \
        {                         \
            msg = "";             \
        }                         \
        msg;                      \
    })

#define at_set_mqtt_ver "AT+QMTCFG=\"version\",%d,4"              // mqtt idx
#define at_set_recv_mode "AT+QMTCFG=\"recv/mode\",%d,1,1"         //mqtt index, store mode
#define at_set_ssl "AT+QMTCFG=\"SSL\",%d,%d,%d"                   //idx=1 ssl_idx=2
#define at_set_ca_cert "AT+QSSLCFG=\"cacert\",%d,\"UFS:%s\""      //ssl_idx=2 cacert.pem
#define at_set_ssl_level "AT+QSSLCFG=\"seclevel\",%d,1"           //0:no, 1:server, 2:server&client                                   //ssl_idx=2
#define at_set_ssl_version "AT+QSSLCFG=\"sslversion\",%d,3"       //ssl_idx=2
#define at_set_ciphersuite "AT+QSSLCFG=\"ciphersuite\",%d,0xFFFF" //ssl_idx=2
#define at_ssl_ignore_ltime "AT+QSSLCFG=\"ignorelocaltime\",%d,1" //ssl_idx=2
#define at_set_keepalive "AT+QMTCFG=\"keepalive\",%d,180"         //idx
// #define at_mqtt_open_aiswei "AT+QMTOPEN=%d,\"mqttstls.heclouds.com\",%d" //idx
#define at_mqtt_open "AT+QMTOPEN=%d,\"%s\",%d"               //idx url
#define at_mqtt_connect "AT+QMTCONN=%d,\"%s\",\"%s\",\"%s\"" //idx
#define at_mqtt_sub "AT+QMTSUB=%d,123,\"%s\",0"              //idx
#define at_mqtt_pub "AT+QMTPUBEX=%d,%d,0,0,\"%s\",%d"        //idx qos=0
#define at_mqtt_pub_qos0 "AT+QMTPUBEX=%d,%d,0,0,\"%s\",%d"   //idx qos=0
#define at_mqtt_disc "AT+QMTDISC=%d"
#define at_mqtt_close "AT+QMTCLOSE=%d" //idx

#define at_mqtt_recv_check "AT+QMTRECV?"
#define at_mqtt_recv_read "AT+QMTRECV=%d,%d" //0~4 //idx=fst

/** http commands ************************************************************************************/
#define at_http_response_head_off "AT+QHTTPCFG=\"responseheader\",0"
#define at_http_response_head_on "AT+QHTTPCFG=\"responseheader\",1"
#define at_http_request_head_off "AT+QHTTPCFG=\"requestheader\",0"
#define at_http_request_head_on "AT+QHTTPCFG=\"requestheader\",1"
#define at_http_id "AT+QHTTPCFG=\"contextid\",1"
#define at_http_url "AT+QHTTPURL=%d,5"
#define at_http_get_action "AT+QHTTPGET=20"
#define at_http_user_get "AT+QHTTPGET=60,%d" //request head size
#define at_http_read "AT+QHTTPREAD=5"
#define at_http_release "AT+QHTTPSTOP"

#define RANGE_HEAD_FMT "GET %s HTTP/1.1\r\nHost: %s\r\nRange: bytes=%d-%d\r\n\r\n"
#define KB_N 5

/** *************************************************************************************************/

#define RAW "\033[0m"
#define RED "\033[1;31m"
#define YELLOW "\033[1;33m"
#define BLUE "\033[1;34m"
#define GREEN "\033[1;32m"

static int g_mqtt_ok[2] = {0};
static int g_data_call_ok = 0;
static int g_cereg_ok = 0;
static int g_cgreg_ok = 0;
static int g_net_mode = -1;
static int g_csq = -1;
static int g_tcp_status = 0;

static int is_downloading = 0;

/** 用于LED灯、重新拨号、TCP重连*/
int cat1_get_csq(void)
{
    return g_csq;
}

int cat1_get_cereg_status(void)
{
    return g_cereg_ok;
}

int cat1_get_cgreg_status(void)
{
    return g_cgreg_ok;
}

int cat1_get_net_mode_value(void)
{
    return g_net_mode;
}

void cat1_set_data_call_ok(void)
{
    g_data_call_ok = 1;
}

void cat1_set_data_call_error(void)
{
    g_data_call_ok = 0;
}

int cat1_get_data_call_status(void)
{
    return g_data_call_ok;
}

void cat1_set_mqtt_ok(int index)
{
    g_mqtt_ok[index] = 1;
}

void cat1_set_mqtt_error(int index)
{
    g_mqtt_ok[index] = 0;
}

int cat1_get_mqtt_status(int index)
{
    return g_mqtt_ok[index];
}

/** ********************************************/

void set_print_color(char *color)
{
    printf("%s", color);
}

void set_print_blue(void)
{
    set_print_color(BLUE);
}

void red_log(char *msg)
{
    set_print_color(RED);
    printf("%s\n", msg);
    set_print_color(BLUE);
}

void yellow_log(char *msg)
{
    set_print_color(YELLOW);
    printf("%s\n", msg);
    set_print_color(BLUE);
}

/** uart send and recv log*/
static int frame_recv(uint8_t *buf, int buf_size)
{
    // char buf[1536] = {0};
    int len = 0;
    int nread = 0;
    int timeout_count = 0;
    int nleft = buf_size;

    while (1)
    {
        usleep(50000);
        timeout_count++;
        if (timeout_count > 20) /** 20 = 1 second*/
        {
            return 0;
        }

        nleft = buf_size - len - 1;
        nread = uart_read_bytes(CAT1_UART, buf + len, nleft, 0);
        if (nread > 0)
        {
            len += nread;
            break;
        }
    }

    while (1)
    {
        usleep(50000);
        nleft = buf_size - len - 1;
        if (nleft > 0)
        {
            nread = uart_read_bytes(CAT1_UART, buf + len, nleft, 0);
            if (nread > 0)
            {
                len += nread;
                continue;
            }
        }

        // memcpy(_buf, buf, len);
        return len;
    }
}

static int frame_recv_1ms(uint8_t *buf, int buf_size)
{
    // char buf[1536] = {0};
    int len = 0;
    int nread = 0;
    int timeout_count = 0;
    int nleft = buf_size;

    while (1)
    {
        usleep(50000);
        timeout_count++;
        if (timeout_count > 60) /** 20 = 1 second*/
        {
            return 0;
        }

        nleft = buf_size - len - 1;
        nread = uart_read_bytes(CAT1_UART, buf + len, nleft, 0);
        if (nread > 0)
        {
            len += nread;
            break;
        }
    }

    while (1)
    {
        usleep(1000);
        nleft = buf_size - len - 1;
        if (nleft > 0)
        {
            nread = uart_read_bytes(CAT1_UART, buf + len, nleft, 0);
            if (nread > 0)
            {
                len += nread;
                continue;
            }
        }

        // memcpy(_buf, buf, len);
        return len;
    }
}

/** 接收一帧，并过滤检查通知类信息*/
static int frame_filter(char *buf, int buf_size)
{
    // char buf[1536] = {0};
    // at_response_msg_t msg = {0};
    char *p1;
    char *p2;
    int recv_len = frame_recv(buf, buf_size);
    if (recv_len > 0)
    {
        if (is_downloading == 0)
        {
            printf("<RECV FRAME==============>\n");
            for (int i = 0; i < recv_len; i++)
                printf("%02X ", (uint8_t)buf[i]);
            printf("\n</RECV FRAME==============>\n");

            if (FAST_LOG_EN == 1)
            {
                printf("RECV FRAME: %d <RECV STRING FRAME>%.*s</RECV STRING FRAME>\n", recv_len, recv_len, buf);
            }

            /** tcp close通知*/
            char *tcp_tag = "\r\n+QIURC: \"closed\",0";
            p1 = memmem(buf, recv_len, tcp_tag, strlen(tcp_tag));
            if (p1 != NULL)
            {
                printf("FRAME FILTER: found tcp close *******************\n");
                cat1_set_tcp_error();
            }

            /** MQTT离线*/
            char *mqtt_tag = "\r\n+QMTSTAT:";
            p1 = memmem(buf, recv_len, mqtt_tag, strlen(mqtt_tag));
            if (p1 != NULL)
            {
                int mqtt_urc = 0;
                int mqtt_idx = 0;
                printf("FRAME FILTER: found +QMTSTAT *******************\n");
                int n_parsed = sscanf(p1, "\r\n+QMTSTAT: %d,%d\r\n", &mqtt_idx, &mqtt_urc);
                if (n_parsed == 2 && mqtt_urc > 0)
                {
                    cat1_set_mqtt_error(mqtt_idx);
                }
            }

            char *start_tag = "\r\nRDY\r\n";
            p1 = memmem(buf, recv_len, start_tag, strlen(start_tag));
            if (p1 != NULL)
            {
                printf("FRAME FILTER: found +RDY *******************\n");
                sleep(1);
                int res = cat1_echo_disable();
                printf("\nRDY, disable AT echo ok, res=%d**************************************+++\n", res);
            }
        }

        // /** 驻网信息，不用管*/
        // if (strstr(buf, "\r\n+CGREG:") != NULL)
        // {
        //     ;
        // }

        // memcpy(msg, buf, recv_len);
        return recv_len;
    }
    else
    {
        return 0;
    }
}

static int at_recv(char *buf, int buf_size)
{
    return frame_filter(buf, buf_size);
}

static void at_send(char *cmd)
{
    char buf[1536] = {0};
    int len = strlen(cmd);
    memcpy(buf, cmd, len);
    memcpy(buf + len, "\r", 1);
    len = len + 1;

    uart_flush(CAT1_UART);
    uart_write_bytes(CAT1_UART, buf, len);
    if (FAST_LOG_EN == 1)
    {
        if (strncmp(buf, "AT+QMTRECV?", strlen("AT+QMTRECV?")) != 0)
            printf("SERIAL AT SEND: %s\n", buf);
    }
}

static void raw_send(char *cmd, int len)
{
    uart_flush(CAT1_UART);
    uart_write_bytes(CAT1_UART, cmd, len);
    printf("SERIAL SEND: %.*s\n", len, cmd);
    int i = 0;
    printf("\n<SEND>\n");
    for (int i = 0; i < len; i++)
        printf("%02X ", (uint8_t)cmd[i]);
    printf("\n</SEND>\n");
}

/** 接收AT指令回复，匹配关键字，10秒超时*/
//-0: ok
//-1: error
//-2: timeout
static int multi_keyword_check(int argc, char **argv, int sec)
{
    char buf[1536] = {0};
    int size = 0;
    int count = 0;
    static int index = 0;
    int i = 0;

    //1 = 1 sec
    int start_sec = get_second_sys_time();
    while (get_second_sys_time() - start_sec <= sec)
    {
        count++;
        memset(buf, 0, sizeof(buf));
        size = at_recv(buf, sizeof(buf));
        if (size > 0)
        {
            for (i = 0; i < argc; i++)
            {
                if (strstr(buf, argv[i]) != NULL)
                {
                    int res = 0;
                    if (i == 0)
                    {
                        res = 0;
                    }
                    else
                    {
                        res = -1;
                    }
                    set_print_color(YELLOW);
                    printf("keyword check ok: %s --------------------(%d)\n", argv[i], index++);
                    set_print_color(BLUE);
                    return res;
                }
            }
        }
    }
    set_print_color(RED);
    printf("keyword check timeout: ");
    for (i = 0; i < argc; i++)
    {
        printf("%s, ", argv[i]);
        printf("\n");
    }
    set_print_color(BLUE);
    // exit(0);
    return -2;
}

static int keyword_check(char *keyword)
{
    char *s1[] = {keyword, "ERROR"};
    return multi_keyword_check(2, s1, 11);
}

static int keyword_timeout_check(char *keyword, int sec)
{
    char *s1[] = {keyword, "ERROR"};
    return multi_keyword_check(2, s1, sec);
}

static int fast_keyword_check(char *keyword)
{
    char *s1[] = {keyword, "ERROR"};
    return multi_keyword_check(2, s1, 4);
}

char g_ip[17] = {0};
static int get_data_call_ip(char *ip)
{
    char buf[1536] = {0};
    int size = 0;
    int count = 0;
    char *keyword = "+QIACT:";
    //3 = 3 sec
    while (count < 3)
    {
        count++;
        memset(buf, 0, sizeof(buf));
        size = at_recv(buf, sizeof(buf));
        printf("get_data_call_ip: count=%d size=%d %s\n", count, size, buf);
        if (size > 0)
        {
            char *p1 = strstr(buf, keyword);
            if (p1 != NULL)
            {
                int state = -1;
                char my_ip[17] = {0};
                int num = sscanf(p1, "+QIACT: %*d,%d,%*d,\"%[^\"]", &state, my_ip);
                printf("22222222222222222222222sss\n");
                if (num == 2)
                {
                    if (state == 1)
                    {
                        if (ip != NULL)
                        {
                            memcpy(ip, my_ip, strlen(my_ip));
                        }
                        memcpy(g_ip, my_ip, strlen(my_ip));

                        set_print_color(YELLOW);
                        printf("data call already ok, ip: %s\n", my_ip);
                        set_print_color(BLUE);
                        return 0;
                    }
                    else
                    {
                        printf("data call loss\n");
                        at_send(at_data_call_clear);
                        keyword_check("OK");
                        return -1;
                    }
                }
                else
                {
                    printf("3333333333333333333333333ssss\n");
                    return -1;
                }
            }
            else
            {
                printf("11111111111111111111111111111sss\n");
                p1 = strstr(buf, "OK");
                if (p1 != NULL)
                    return -1;
            }
        }
    }

    set_print_color(RED);
    printf("data call state check timeout: %s\n", keyword);
    set_print_color(BLUE);
    // exit(0);
    return -1;
}

static int csq_check(void)
{
    char buf[1536] = {0};
    int size = 0;
    int count = 0;
    char *keyword = "\r\n+CSQ:";

    /** 1 = 1 sec*/
    while (count < 3)
    {
        count++;
        memset(buf, 0, sizeof(buf));
        size = at_recv(buf, sizeof(buf));
        if (size > 0)
        {
            char *p1 = strstr(buf, keyword);
            if (p1 != NULL)
            {
                int csq = -1;
                int num = sscanf(p1, "\r\n+CSQ: %d,", &csq);
                if (num == 1 && csq >= 0 && csq <= 31)
                {
                    set_print_color(YELLOW);
                    printf("CSQ = %d **************\n", csq);
                    set_print_color(BLUE);
                    return csq;
                }
                else
                {
                    return -1;
                }
            }
            else
            {
                p1 = strstr(buf, "ERROR");
                if (p1 != NULL)
                {
                    return -1;
                }
            }
        }
    }
    set_print_color(RED);
    printf("csq check timeout: %s\n", keyword);
    set_print_color(BLUE);
    // exit(0);
    return -1;
}

unsigned int _str2hex(unsigned char *ibuf, unsigned char *obuf,
                      unsigned int olen)
{
    unsigned int i;      /* loop iteration variable */
    unsigned int j = 0;  /* current character */
    unsigned int by = 0; /* byte value for conversion */
    unsigned char ch;    /* current character */
    unsigned int len = strlen((char *)ibuf);

    /* process the list of characaters */
    for (i = 0; i < len; i++)
    {
        if (i == (2 * olen))
        {
            // truncated it.
            return j + 1;
        }
        ch = ibuf[i];
        /* do the conversion */
        if (ch >= '0' && ch <= '9')
            by = (by << 4) + ch - '0';
        else if (ch >= 'A' && ch <= 'F')
            by = (by << 4) + ch - 'A' + 10;
        else if (ch >= 'a' && ch <= 'f')
            by = (by << 4) + ch - 'a' + 10;
        else
        { /* error if not hexadecimal */
            return 0;
        }

        /* store a byte for each pair of hexadecimal digits */
        if (i & 1)
        {
            j = ((i + 1) / 2) - 1;
            obuf[j] = by & 0xff;
        }
    }
    return j + 1;
}

int esp_str2hex(uint8_t *_in, uint8_t *_out)
{
    int in_len = 0;
    uint8_t in[50] = {0};
    if (strlen(_in) % 2 > 0)
    {
        sprintf(in, "0%s", _in);
    }
    else
    {
        sprintf(in, "%s", _in);
    }
    in_len = strlen(in);

    int res_len = 0;

    res_len = _str2hex(in, _out, in_len / 2);
    return res_len;
}

static int creg_check(uint8_t *olac, uint8_t *oci, int *lac_len, int *ci_len)
{
    char buf[1536] = {0};
    int size = 0;
    int count = 0;
    char *keyword = "\r\n+CREG:";
    char lac[20] = {0};
    char ci[20] = {0};

    /** 1 = 1 sec*/
    while (count < 3)
    {
        count++;
        memset(buf, 0, sizeof(buf));
        size = at_recv(buf, sizeof(buf));
        if (size > 0)
        {
            char *p1 = strstr(buf, keyword);
            if (p1 != NULL)
            {
                int num = sscanf(p1, "\r\n+CREG: %*d,%*d,\"%[^\"]", lac);
                if (num == 1)
                {
                    set_print_color(YELLOW);
                    printf("creg lac: %s **************\n", lac);
                    set_print_color(BLUE);
                    *lac_len = esp_str2hex(lac, olac);

                    char *tag = "\",\"";
                    char *p = strstr(p1, tag);
                    p = p + strlen(tag);
                    num = sscanf(p, "%[^\"]", ci);
                    if (num == 1)
                    {
                        printf("ci:%s\n", ci);
                        printf("ci len:%d\n", strlen(ci));
                        *ci_len = esp_str2hex(ci, oci);
                        printf("get lac ci success: %s, %s\n", lac, ci);
                        return 0;
                    }
                }
                else
                {
                    return -1;
                }
            }
            else
            {
                p1 = strstr(buf, "ERROR");
                if (p1 != NULL)
                {
                    return -1;
                }
            }
        }
    }
    set_print_color(RED);
    printf("creg check timeout: %s\n", keyword);
    set_print_color(BLUE);
    // exit(0);
    return -1;
}

static int cereg_check(void)
{
    char buf[1536] = {0};
    int size = 0;
    int count = 0;
    char *keyword = "\r\n+CEREG:";

    /** 1 = 1 sec*/
    while (count < 3)
    {
        count++;
        memset(buf, 0, sizeof(buf));
        size = at_recv(buf, sizeof(buf));
        if (size > 0)
        {
            char *p1 = strstr(buf, keyword);
            if (p1 != NULL)
            {
                int reg = -1;
                int num = sscanf(p1, "\r\n+CEREG: %*d,%d", &reg);
                if (num == 1 && (reg == 1 || reg == 5))
                {
                    set_print_color(YELLOW);
                    printf("CEREG OK: stat=%d **************\n", reg);
                    set_print_color(BLUE);
                    return 0;
                }
                else
                {
                    return -1;
                }
            }
            else
            {
                p1 = strstr(buf, "ERROR");
                if (p1 != NULL)
                {
                    return -1;
                }
            }
        }
    }
    set_print_color(RED);
    printf("cereg check timeout: %s\n", keyword);
    set_print_color(BLUE);
    // exit(0);
    return -1;
}

static int cgreg_check(void)
{
    char buf[1536] = {0};
    int size = 0;
    int count = 0;
    char *keyword = "\r\n+CGREG:";

    /** 1 = 1 sec*/
    while (count < 3)
    {
        count++;
        memset(buf, 0, sizeof(buf));
        size = at_recv(buf, sizeof(buf));
        if (size > 0)
        {
            char *p1 = strstr(buf, keyword);
            if (p1 != NULL)
            {
                int reg = -1;
                int num = sscanf(p1, "\r\n+CGREG: %*d,%d", &reg);
                if (num == 1 && (reg == 1 || reg == 5))
                {
                    set_print_color(YELLOW);
                    printf("CGREG OK: stat=%d **************\n", reg);
                    set_print_color(BLUE);
                    return 0;
                }
                else
                {
                    return -1;
                }
            }
            else
            {
                p1 = strstr(buf, "ERROR");
                if (p1 != NULL)
                {
                    return -1;
                }
            }
        }
    }
    set_print_color(RED);
    printf("cgreg check timeout: %s\n", keyword);
    set_print_color(BLUE);
    // exit(0);
    return -1;
}

static int net_info_check(char *oper, int *mode)
{
    char buf[1536] = {0};
    int size = 0;
    int count = 0;
    char *keyword = "+COPS:";

    /** 1 = 1 sec*/
    while (count < 3)
    {
        count++;
        memset(buf, 0, sizeof(buf));
        size = at_recv(buf, sizeof(buf));
        if (size > 0)
        {
            char *p1 = strstr(buf, keyword);
            if (p1 != NULL)
            {
                int my_mode = -1;
                char my_oper[30] = {0}; //
                int num = sscanf(p1, "%*[^\"]\"%[^\",]\",%d", my_oper, &my_mode);
                if (num == 2)
                {
                    if (oper != NULL)
                        memcpy(oper, my_oper, strlen(my_oper));
                    *mode = my_mode;
                    set_print_color(YELLOW);
                    printf("OPER: %s ***\n", my_oper);
                    printf("NET_MODE: %d ***\n", my_mode);
                    set_print_color(BLUE);
                    return 0;
                }
                else
                {
                    return -1;
                }
            }
            else
            {
                p1 = strstr(buf, "ERROR");
                if (p1 != NULL)
                {
                    return -1;
                }
            }
        }
    }
    set_print_color(RED);
    printf("net_info_check timeout: %s\n", keyword);
    set_print_color(BLUE);
    // exit(0);
    return -1;
}

static int mqtt_sub_check(void)
{
    char buf[1536] = {0};
    int size = 0;
    int count = 0;
    char *keyword = "+QMTSUB: ";

    //1 = 1 sec
    while (count < 5)
    {
        count++;
        memset(buf, 0, sizeof(buf));
        size = at_recv(buf, sizeof(buf));
        if (size > 0)
        {
            char *p1 = strstr(buf, keyword);
            if (p1 != NULL)
            {
                int result = -1;
                int num = sscanf(p1, "+QMTSUB: %*d,%*d,%d", &result);
                if (num == 1 && result == 0)
                {
                    set_print_color(YELLOW);
                    printf("mqtt sub check ok\n");
                    set_print_color(BLUE);
                    return 0;
                }
                else
                {
                    return -1;
                }
            }
        }
    }
    set_print_color(RED);
    printf("mqtt sub check timeout: %s\n", keyword);
    set_print_color(BLUE);
    // exit(0);
    return -1;
}

static int mqtt_pub_check(uint16_t *_msg_id)
{
    char buf[1536] = {0};
    int size = 0;
    int count = 0;
    char *keyword = "+QMTPUBEX: ";
    //1 = 1 sec
    while (count < 15) //5sec * 3times = 15sec
    {
        count++;
        memset(buf, 0, sizeof(buf));
        size = at_recv(buf, sizeof(buf));
        if (size > 0)
        {
            char *p1 = NULL;
            p1 = strstr(buf, keyword);
            if (p1 != NULL)
            {
                uint16_t msg_id = 0;
                int pub_res = -1;

                int num = sscanf(p1, "+QMTPUBEX: %*d,%hu,%d", &msg_id, &pub_res);
                if (num == 2)
                {
                    if (pub_res == 0 || pub_res == 1)
                    {
                        set_print_color(YELLOW);
                        printf("mqtt pub check ok: msg_id=%hu, return=%d\n", msg_id, pub_res);
                        *_msg_id = msg_id;
                        set_print_color(BLUE);
                        return 0;
                    }
                    else
                    {
                        return -1;
                    }
                }
                else
                {
                    return -1;
                }
            }
        }
    }
    set_print_color(RED);
    printf("mqtt pub check timeout: %s\n", keyword);
    set_print_color(BLUE);
    // exit(0);
    return -1;
}

static int get_mqtt_recv_index(int *index)
{
    char buf[1536] = {0};
    int size = 0;
    int count = 0;
    int row = 0;
    char *keyword = "\r\n+QMTRECV:";

    //1 = 1 sec
    while (count < 3)
    {
        count++;
        memset(buf, 0, sizeof(buf));
        size = at_recv(buf, sizeof(buf));
        if (size > 0)
        {
            char *p1 = buf;
            while (1)
            {
                p1 = strstr(p1, keyword);
                if (p1 != NULL)
                {
                    int arr[5] = {0};
                    int num = sscanf(p1, "\r\n+QMTRECV: %d,%d,%d,%d,%d,%d", index, &arr[0], &arr[1], &arr[2], &arr[3], &arr[4]);
                    if (num == 6)
                    {
                        int i = 0;
                        for (i = 0; i < 5; i++)
                        {
                            if (arr[i] == 1)
                                return i;
                            if (i == 4) // 1st row no msg, search 2nd row
                            {
                                // printf("mqtt recv check, row=%d, no msg ***************\n", row);
                                row++;
                                p1 = p1 + strlen(keyword);
                                continue;
                            }
                        }
                    }
                    else
                    {
                        return -1;
                    }
                }
                else
                {
                    return -1;
                }
            }
        }
    }
    set_print_color(RED);
    printf("mqtt_recv_check_index timeout\n");
    set_print_color(BLUE);
    return -1;
}

static int mqtt_recv_parse(int index)
{
    char buf[1536] = {0};
    int size = 0;
    int count = 0;
    char *keyword = "\r\n+QMTRECV:";

    //1 = 1 sec
    while (count < 3)
    {
        count++;
        memset(buf, 0, sizeof(buf));
        size = at_recv(buf, sizeof(buf));
        if (size > 0)
        {
            char *p1 = strstr(buf, keyword);
            if (p1 != NULL)
            {
                char topic[200] = {0};
                char payload[1536] = {0};
                int parsed_num = 0;

                printf("<buf>%s</buf>\n", buf);

                parsed_num = sscanf(buf, "%*[^\"]\"%[^\",]\",%*d", topic);

                p1 = strstr(buf, "\"{");
                char *p2 = strstr(buf, "}\"");
                if (p1 != NULL && p2 != NULL)
                {
                    memcpy(payload, p1 + 1, p2 - p1);
                }
                else
                {
                    return -1;
                }

                if (parsed_num == 1)
                {
                    set_print_color(BLUE);
                    printf("sub recv sub topic: %s\n", topic);
                    printf("sub recv payload: %s\n", payload);
                    set_print_color(BLUE);

                    /** 根据topic类型分别处理*/
                    if (index == 1)
                    {
                        if (strstr(topic, "rrpc") != NULL)
                        {
                            parse_mqtt_msg_aliyun(topic, strlen(topic), payload, strlen(payload));
                            return 0;
                        }
                    }
                }
                else
                {
                    return -1;
                }
            }
            else
            {
                return -1;
            }
        }
    }
    set_print_color(RED);
    printf("mqtt_recv_parse timeout\n");
    set_print_color(BLUE);
    return -1;
}

int cat1_mqtt_recv(void)
{
    if (cat1_get_mqtt_status(1) == 0)
        return -1;
    at_send(at_mqtt_recv_check);
    int mqtt_index = -1;
    int index = get_mqtt_recv_index(&mqtt_index);
    if (index < 0 || index > 4 || mqtt_index > 1 || mqtt_index < 0)
        return -1;

    char cmd[100] = {0};
    sprintf(cmd, at_mqtt_recv_read, mqtt_index, index);
    at_send(cmd);
    return mqtt_recv_parse(mqtt_index);
}

static int http_clear(void)
{
    int res = -1;
    at_send(at_http_release);
    res = keyword_check("OK");
    return res;
}

static int ccid_check(char *ccid)
{
    char buf[1536] = {0};
    int size = 0;
    int count = 0;
    char *keyword = "\r\n+QCCID:";

    //1 = 1 sec
    while (count < 2)
    {
        count++;
        memset(buf, 0, sizeof(buf));
        size = at_recv(buf, sizeof(buf));
        if (size > 0)
        {
            char *p1 = strstr(buf, keyword);
            if (p1 != NULL)
            {
                int res = 0;
                int parsed_num = 0;

                parsed_num = sscanf(p1, "\r\n+QCCID: %s", ccid);
                if (parsed_num == 1)
                {
                    return 0;
                }
                else
                {
                    return -1;
                }
            }
        }
    }
    set_print_color(RED);
    printf("ccid_check timeout\n");
    set_print_color(BLUE);
    return -1;
}

static int imei_or_imsi_check(char *data)
{
    char buf[1536] = {0};
    int size = 0;
    int count = 0;
    char *keyword = "\r\n";

    //1 = 1 sec
    while (count < 2)
    {
        count++;
        memset(buf, 0, sizeof(buf));
        size = at_recv(buf, sizeof(buf));
        if (size > 0)
        {
            char *p1 = strstr(buf, keyword);
            if (p1 != NULL)
            {
                int res = 0;
                int parsed_num = 0;

                parsed_num = sscanf(p1, "\r\n%s", data);
                if (parsed_num == 1)
                {
                    return 0;
                }
                else
                {
                    return -1;
                }
            }
        }
    }
    set_print_color(RED);
    printf("imei_or_imsi_check timeout\n");
    set_print_color(BLUE);
    return -1;
}

int cat1_get_ccid(char *ccid)
{
    int res = -1;
    at_send(at_qccid);
    res = ccid_check(ccid);
    return res;
}

int cat1_get_imei(char *imei)
{
    int res = -1;
    at_send(at_imei);
    res = imei_or_imsi_check(imei);
    return res;
}

int cat1_get_imsi(char *imsi)
{
    int res = -1;
    at_send(at_imsi);
    res = imei_or_imsi_check(imsi);
    return res;
}

int cat1_tcp_open_old(char *host, int port)
{
    int res = -1;
    char buf[200] = {0};
    sprintf(buf, at_qiopen, host, port);
    at_send(buf);
    res = keyword_check("+QIOPEN: 0,0"); //3 sec timeout
    return res;
}

int cat1_tcp_open(char *host, int port)
{
    int res = -1;
    char buf[200] = {0};

    at_send(at_ssl_version);
    res = keyword_check("OK");
    if (res != 0)
        return -1;
    at_send(at_ssl_cipher);
    res = keyword_check("OK");
    if (res != 0)
        return -1;

    at_send(at_ssl_level);
    res = keyword_check("OK");
    if (res != 0)
        return -1;

    memset(buf, 0, sizeof(buf));
    sprintf(buf, at_ssl_ignore_valid, 0);
    at_send(buf);
    res = keyword_check("OK");

    memset(buf, 0, sizeof(buf));
    sprintf(buf, at_ssl_ignore_ltime, 0);
    at_send(buf);
    res = keyword_check("OK");

    memset(buf, 0, sizeof(buf));
    sprintf(buf, at_ssl_ignore_chain, 0);
    at_send(buf);
    res = keyword_check("OK");

    res = cat1_cacaert_set(0);
    if (res != 0)
    {
        red_log("ca cert set error: server");
        return -1;
    }

    res = cat1_upload_cli_crt();
    if (res != 0)
    {
        red_log("cli crt set error: server");
        return -1;
    }

    res = cat1_upload_cli_key();
    if (res != 0)
    {
        red_log("cli key set error: server");
        return -1;
    }

    memset(buf, 0, sizeof(buf));
    sprintf(buf, at_set_ca_cert, 0, ssl_name_tab[0]);
    at_send(buf);
    res = keyword_check("OK");
    if (res != 0)
        return -1;

    memset(buf, 0, sizeof(buf));
    sprintf(buf, at_ssl_client_crt, cli_name_tab[0]);
    at_send(buf);
    res = keyword_check("OK");
    if (res != 0)
        return -1;

    memset(buf, 0, sizeof(buf));
    sprintf(buf, at_ssl_client_key, cli_name_tab[1]);
    at_send(buf);
    res = keyword_check("OK");
    if (res != 0)
        return -1;

    sprintf(buf, at_ssl_open, host, port);
    at_send(buf);
    res = keyword_check("+QSSLOPEN: 0,0"); //3 sec timeout
    return res;
}

int cat1_tcp_close_old(void)
{
    int res = -1;
    at_send(at_qiclose);
    res = keyword_check("OK"); //3 sec timeout
    return res;
}

int cat1_tcp_close(void)
{
    int res = -1;
    at_send(at_ssl_close);
    res = keyword_check("OK"); //3 sec timeout
    return res;
}

static int tcp_state_recv_old(void)
{
    char buf[1536] = {0};
    int size = 0;
    int count = 0;
    char *keyword = "+QISTATE: 0,\"TCP\",";
    char *fmt = "+QISTATE: %*d,\"TCP\",\"%*d.%*d.%*d.%*d\",%*d,%*d,%d";
    int state = 0;
    char *exit_word = "OK";

    //1 = 1 sec
    while (count < 2)
    {
        count++;
        memset(buf, 0, sizeof(buf));
        size = at_recv(buf, sizeof(buf));
        if (size > 0)
        {
            char *p1 = strstr(buf, keyword);
            char *p2 = strstr(buf, exit_word);
            if (p1 != NULL)
            {
                int res = 0;
                int parsed_num = 0;

                parsed_num = sscanf(p1, fmt, &state);
                if (parsed_num == 1 && state == 2)
                {
                    return 0;
                }
                else
                {
                    return -1;
                }
            }
            else if (p2 != NULL)
            {
                set_print_color(RED);
                printf("no tcp socket\n");
                set_print_color(BLUE);
                return -1;
            }
        }
    }
    set_print_color(RED);
    printf("tcp_state_recv timeout\n");
    set_print_color(BLUE);
    return -1;
}

static int tcp_state_recv(void)
{
    char buf[1536] = {0};
    int size = 0;
    int count = 0;
    char *keyword = "+QSSLSTATE: 0,\"SSLClient\",";
    char *fmt = "+QSSLSTATE: %*d,\"SSLClient\",\"%*d.%*d.%*d.%*d\",%*d,%*d,%d";
    int state = 0;
    char *exit_word = "OK";

    //1 = 1 sec
    while (count < 2)
    {
        count++;
        memset(buf, 0, sizeof(buf));
        size = at_recv(buf, sizeof(buf));
        if (size > 0)
        {
            char *p1 = strstr(buf, keyword);
            char *p2 = strstr(buf, exit_word);
            if (p1 != NULL)
            {
                int res = 0;
                int parsed_num = 0;

                parsed_num = sscanf(p1, fmt, &state);
                if (parsed_num == 1 && state == 2)
                {
                    return 0;
                }
                else
                {
                    return -1;
                }
            }
            else if (p2 != NULL)
            {
                set_print_color(RED);
                printf("no tcp socket\n");
                set_print_color(BLUE);
                return -1;
            }
        }
    }
    set_print_color(RED);
    printf("tcp_state_recv timeout\n");
    set_print_color(BLUE);
    return -1;
}

int cat1_tcp_state_check(void)
{
    int res = -1;
    at_send(at_ssl_state);
    res = tcp_state_recv(); //3 sec timeout
    if (res == 0)
        g_tcp_status = 1;
    else
    {
        g_tcp_status = 0;
    }
    return res;
}

int cat1_get_tcp_status(void)
{
    return g_tcp_status;
}

void cat1_set_tcp_ok(void)
{
    g_tcp_status = 1;
}

void cat1_set_tcp_error(void)
{
    g_tcp_status = 0;
}

int cat1_tcp_send_old(char *buf, int len)
{
    int res = -1;
    char cmd[1536] = {0};
    sprintf(cmd, at_qisend, len);
    at_send(cmd);

    res = keyword_check("\r\n>");
    if (res != 0)
        return -1;

    raw_send(buf, len);
    res = keyword_check("SEND OK");
    if (res != 0)
        return -1;

    return 0;
}

int cat1_tcp_send(char *buf, int len)
{
    int res = -1;
    char cmd[1536] = {0};
    sprintf(cmd, at_ssl_send, len);
    at_send(cmd);

    res = keyword_check("\r\n>");
    if (res != 0)
        return -1;

    raw_send(buf, len);
    res = keyword_check("SEND OK");
    if (res != 0)
        return -1;

    return 0;
}

/** 单次+QIRD指令回复接收，有回复则完整接收一次*/
int tcp_recv_check_old(char *msg)
{
    int res = -1;
    char buf[1536] = {0};
    int size = 0;
    int count = 0;
    char *keyword = "+QIRD: ";
    char *fmt = "+QIRD: %d";
    int body_len = 0;
    int real_len = 0;

    int start_sec = get_second_sys_time();
    while (get_second_sys_time() - start_sec < 5) /** 读取无响应的超时: 5sec*/
    {
        memset(buf, 0, sizeof(buf));
        size = at_recv(buf, sizeof(buf));
        if (size > 0)
        {
            char *p1 = strstr(buf, keyword);
            if (p1 != NULL) /** 如果是有效回复*/
            {
                int res = 0;
                int parsed_num = 0;

                parsed_num = sscanf(p1, fmt, &body_len);
                if (parsed_num == 1) /** 如果是有效回复*/
                {
                    if (body_len > 0)
                    {
                        char *newline = "\r\n";
                        p1 = strstr(p1, newline);
                        if (p1 != NULL)
                        {
                            p1 = p1 + strlen(newline);
                            int this_len = buf + size - p1;
                            if (this_len >= body_len) /** 如果此帧包含完整回复：直接拷贝，返回*/
                            {
                                memcpy(msg + real_len, p1, body_len);
                                return body_len;
                            }
                            else /** 如果此帧不够完整回复：继续接收*/
                            {
                                memcpy(msg + real_len, p1, this_len); /** 此帧部分暂存*/
                                real_len += this_len;
                                start_sec = get_second_sys_time(); /** 重新计时*/
                                while (1)
                                {
                                    if (get_second_sys_time() - start_sec > 3000) /** 接收剩余部分，发生超时*/
                                        return 0;
                                    memset(buf, 0, sizeof(buf));
                                    size = at_recv(buf, sizeof(buf));
                                    if (size > 0)
                                    {
                                        start_sec = get_second_sys_time(); /** 若每次读到数据，复位计时*/
                                        int left_len = body_len - real_len;
                                        if (size >= left_len) /** 若足够有效长度，返回*/
                                        {
                                            memcpy(msg + real_len, buf, left_len);
                                            real_len += left_len;
                                            return real_len;
                                        }
                                        else /** 若不够有效长度，数据附加，继续接收*/
                                        {
                                            memcpy(msg + real_len, buf, size);
                                            real_len += size;
                                        }
                                    }
                                }
                            }
                        }
                        return 0;
                    }
                    else
                    {
                        set_print_color(GREEN);
                        printf("tcp_recv_check: reponse ok, but no valid data\n");
                        set_print_color(BLUE);
                        return 0;
                    }
                }
            }
        }
    }
    set_print_color(RED);
    printf("tcp_recv_check timeout\n");
    set_print_color(BLUE);
    return 0;
}

int tcp_recv_check(char *msg)
{
    int res = -1;
    char buf[1536] = {0};
    int size = 0;
    int count = 0;
    char *keyword = "+QSSLRECV: ";
    char *fmt = "+QSSLRECV: %d";
    int body_len = 0;
    int real_len = 0;

    int start_sec = get_second_sys_time();
    while (get_second_sys_time() - start_sec < 5) /** 读取无响应的超时: 5sec*/
    {
        memset(buf, 0, sizeof(buf));
        size = at_recv(buf, sizeof(buf));
        if (size > 0)
        {
            char *p1 = strstr(buf, keyword);
            if (p1 != NULL) /** 如果是有效回复*/
            {
                int res = 0;
                int parsed_num = 0;

                parsed_num = sscanf(p1, fmt, &body_len);
                if (parsed_num == 1) /** 如果是有效回复*/
                {
                    if (body_len > 0)
                    {
                        char *newline = "\r\n";
                        p1 = strstr(p1, newline);
                        if (p1 != NULL)
                        {
                            p1 = p1 + strlen(newline);
                            int this_len = buf + size - p1;
                            if (this_len >= body_len) /** 如果此帧包含完整回复：直接拷贝，返回*/
                            {
                                memcpy(msg + real_len, p1, body_len);
                                return body_len;
                            }
                            else /** 如果此帧不够完整回复：继续接收*/
                            {
                                memcpy(msg + real_len, p1, this_len); /** 此帧部分暂存*/
                                real_len += this_len;
                                start_sec = get_second_sys_time(); /** 重新计时*/
                                while (1)
                                {
                                    if (get_second_sys_time() - start_sec > 3000) /** 接收剩余部分，发生超时*/
                                        return 0;
                                    memset(buf, 0, sizeof(buf));
                                    size = at_recv(buf, sizeof(buf));
                                    if (size > 0)
                                    {
                                        start_sec = get_second_sys_time(); /** 若每次读到数据，复位计时*/
                                        int left_len = body_len - real_len;
                                        if (size >= left_len) /** 若足够有效长度，返回*/
                                        {
                                            memcpy(msg + real_len, buf, left_len);
                                            real_len += left_len;
                                            return real_len;
                                        }
                                        else /** 若不够有效长度，数据附加，继续接收*/
                                        {
                                            memcpy(msg + real_len, buf, size);
                                            real_len += size;
                                        }
                                    }
                                }
                            }
                        }
                        return 0;
                    }
                    else
                    {
                        set_print_color(GREEN);
                        printf("tcp_recv_check: reponse ok, but no valid data\n");
                        set_print_color(BLUE);
                        return 0;
                    }
                }
            }
        }
    }
    set_print_color(RED);
    printf("tcp_recv_check timeout\n");
    set_print_color(BLUE);
    return 0;
}

void server_request_handler(void)
{
    char buf[1400] = {0};
    int len = 0;
    if (cat1_get_tcp_status() == 0)
        return;
    cat1_tcp_recv_no_wait(buf, &len);
}

int cat1_tcp_recv_no_wait(char *msg, int *size)
{
    int res = -1;
    // int start_sec = get_second_sys_time();
    // int now_sec = 0;
    char buf[1536] = {0};
    int len = 0;

    if (1)
    {
        // now_sec = get_second_sys_time();
        // if (now_sec - start_sec > 5)
        //     return -1;
        at_send(at_ssl_rd);
        printf("KKKK------------------------------3\n");
        res = tcp_recv_check(msg);
        printf("KKKK------------------------------4\n");
        if (res > 0)
        {
            /** 过滤处理天合服务端的请求，返回服务端的响应:buf, len*/
            printf("KKKK------------------------------5\n");

            printf("<RECV TIANHE FRAME==============>\n");
            for (int i = 0; i < res; i++)
                printf("%02X ", (uint8_t)msg[i]);
            printf("</RECV TIANHE FRAME==============>\n");

            res = tianhe_tcp_filter(msg, res, buf, &len);
            printf("KKKK------------------------------6\n");
            if (res == 0)
            {
                memcpy(msg, buf, len);
                *size = len;
                return 0;
            }
            else
            {
                return -1;
            }
        }
        // sleep(1);
    }
    return -1;
}

int cat1_tcp_recv(char *msg, int *size)
{
    int res = -1;
    int start_sec = get_second_sys_time();
    int now_sec = 0;
    char buf[1536] = {0};
    int len = 0;

    while (1)
    {
        now_sec = get_second_sys_time();
        if (now_sec - start_sec > 5)
            return -1;
        at_send(at_ssl_rd);
        printf("KKKK------------------------------3\n");
        res = tcp_recv_check(msg);
        printf("KKKK------------------------------4\n");
        if (res > 0)
        {
            /** 过滤处理天合服务端的请求，返回服务端的响应:buf, len*/
            printf("KKKK------------------------------5\n");

            printf("<RECV TIANHE FRAME==============>\n");
            for (int i = 0; i < res; i++)
                printf("%02X ", (uint8_t)msg[i]);
            printf("</RECV TIANHE FRAME==============>\n");

            res = tianhe_tcp_filter(msg, res, buf, &len);
            printf("KKKK------------------------------6\n");
            if (res == 0)
            {
                memcpy(msg, buf, len);
                *size = len;
                return 0;
            }
            else
            {
                return -1;
            }
        }
        sleep(1);
    }
    return -1;
}

int cat1_tcp_conn(char *host, int port)
{
    int res = 0;
    cat1_tcp_close();
    res = cat1_tcp_open(host, port);
    if (res == 0)
    {
        g_tcp_status = 1;
        return 0;
    }
    else
    {
        g_tcp_status = 0;
        return -1;
    }
}

/** 读取一次GET响应，并解析有效数据*/
static int http_read_file_check(int *next_begin_addr, int *next_end_addr, char *file_name, int urc_body_size)
{
    char buf[8192] = {0};
    int size = 0;
    // int count = 0;
    static int index = 0;
    int body_len = 0;
    int body_begin_count = 0;
    int body_end_count = 0;
    int file_total_len = 0;
    static int current_len;
    static FILE *fp = NULL;
    int urc_len = 0;
    int nwrite = 0;
    int add_size = 0;
    int double_newline_has_found = 0; //flag
    char *pp1 = NULL;

    //200 = 10 sec
    int start_sec = get_second_sys_time();
    memset(buf, 0, sizeof(buf));
    size = 0;
    /** 完整接收响应，不遗漏*/
    while (1)
    {
        if (get_second_sys_time() - start_sec > 10)
            return -1;
        usleep(50000);
        add_size = at_recv(buf + size, sizeof(buf) - size - 1);
        if (add_size > 0)
        {
            size += add_size;
            printf("add size: %d ****\n", add_size);
            // printf("<RECV>%.*s</RECV>\n", size, buf);
            if (double_newline_has_found == 0)
            {
                pp1 = memmem(buf, size, "\r\n\r\n", 4);
                if (pp1 != NULL)
                {
                    pp1 += 4;
                    double_newline_has_found = 1;
                }
            }
            if (double_newline_has_found == 1)
            {
                if (buf + size - pp1 >= urc_body_size + 23) //+23 tail
                {
                    // printf("<RECV>%.*s</RECV>\n", size, buf);
                    break;
                }
            }
        }
    }

    /** 处理接收内容--------------------------------------------------------------*/
    if (strstr(buf, "HTTP/1.1 206 Partial Content") != NULL)
    {
        set_print_color(YELLOW);
        printf("http recv file check ok --------------------(%d)\n", index++);
        set_print_color(BLUE);

        /** 提取body长度*/
        char *p1 = strstr(buf, "Content-Length: ");
        if (p1 != NULL)
        {
            red_log("+++++++++++++++++++++++++++++++++++++ content-length");
            sscanf(p1, "Content-Length: %d", &body_len);
            p1 = strstr(buf, "Content-Range: ");
            if (p1 != NULL)
            {
                red_log("+++++++++++++++++++++++++++++++++++++ multi para before");
                sscanf(p1, "Content-Range: bytes %d-%d/%d", &body_begin_count, &body_end_count, &file_total_len);
                red_log("+++++++++++++++++++++++++++++++++++++ multi para after");
                /** 提取body内容*/
                p1 = strstr(buf, "\r\n\r\n");
                if (p1 != NULL)
                {
                    red_log("+++++++++++++++++++++++++++++++++++++ double newline");
                    p1 += 4;
                    if (body_begin_count == 0)
                    {
                        if (fp != NULL)
                            fclose(fp);
                        fp = fopen(file_name, "wb+");
                        if (fp == NULL)
                        {
                            force_flash_reformart("storage");
                            printf("http_read_file_check: fopen fail\n");
                            return -1;
                        }
                        current_len = 0;
                        red_log("+++++++++++++++++++++++++++++++++++++ fopen");
                    }
                    if (fp != NULL)
                    {
                        nwrite = fwrite(p1, 1, body_len, fp);
                        if (nwrite != body_len)
                        {
                            fclose(fp);
                            force_flash_reformart("storage");
                            printf("http_read_file_check: fwrite fail\n");
                            return -1;
                        }
                        // set_print_color(RED);
                        // printf("\n<fwrite>%.*s</fwrite>\n", body_len, p1);
                        // set_print_color(BLUE);
                        current_len += body_len;
                        printf("current file len: %d\n", current_len);

                        if (body_end_count >= file_total_len - 1)
                        {
                            yellow_log("GGG-11111111111111");
                            *next_begin_addr = 0;
                            *next_end_addr = 0;
                            fclose(fp);
                            set_print_color(YELLOW);
                            printf("finish file len: %d\n", current_len);
                            set_print_color(BLUE);
                            return 1;
                        }
                        else
                        {
                            yellow_log("GGG-2222222222222");
                            *next_begin_addr = body_end_count + 1;
                            *next_end_addr = body_end_count + 1024 * KB_N; //2048
                            return 0;
                        }
                        printf("--77777\n");
                    }
                    printf("--6666\n");
                }
                printf("--444\n");
            }
            printf("--333\n");
        }
        else
            printf("--88888\n");
    }
    else
        printf("--2222\n");

    printf("--11111111\n");

    if (fp != NULL)
    {
        index = 0;
        current_len = 0;
        fclose(fp);
        fp = NULL;
    }

    printf("http read check timeout.\n");
    // exit(0);
    return -1;
}

static int http_range_head_create(char *head, char *url, int begin, int end)
{
    char host[100] = {0};
    char path[200] = {0};

    char *p1 = NULL;
    char *p2 = NULL;
    p1 = strstr(url, "//");
    if (p1 != NULL)
    {
        p1 += 2;
        p2 = strstr(p1, "/");
        if (p2 != NULL)
        {
            memcpy(host, p1, p2 - p1);
            memcpy(path, p2, strlen(p2));
            sprintf(head, RANGE_HEAD_FMT, path, host, begin, end);
            yellow_log(head);
            return 0;
        }
    }
    red_log("http_range_head_create err");
    return -1;
}

static int http_res_206_len(void)
{
    char buf[1536] = {0};
    int size = 0;
    int count = 0;
    char *keyword = "\r\n+QHTTPGET: 0,206,";
    char *fmt = "\r\n+QHTTPGET: 0,206,%d";

    int start_sec = get_second_sys_time();
    while (get_second_sys_time() - start_sec < 65)
    {
        count++;
        memset(buf, 0, sizeof(buf));
        size = at_recv(buf, sizeof(buf));
        if (size > 0)
        {
            // printf("<>%s</>\n", buf);
            char *p1 = strstr(buf, keyword);
            if (p1 != NULL)
            {
                int http_body_len = -1;
                int num = sscanf(p1, fmt, &http_body_len);
                if (num == 1)
                {
                    set_print_color(YELLOW);
                    printf("http_body_len = %d **************\n", http_body_len);
                    set_print_color(BLUE);
                    return http_body_len;
                }
                else
                {
                    return -1;
                }
            }
            else
            {
                p1 = strstr(buf, "ERROR");
                if (p1 != NULL)
                {
                    return -1;
                }
            }
        }
    }
    set_print_color(RED);
    printf("http_res_206_len timeout: %s\n", keyword);
    set_print_color(BLUE);
    // exit(0);
    return -1;
}

static int http_read_check(char *response)
{
    int count = 0;
    char buf[500] = {0};
    int size;

    /** 1 = 1sec */
    while (count < 3)
    {
        count++;
        memset(buf, 0, sizeof(buf));
        size = at_recv(buf, sizeof(buf));
        if (size > 0)
        {
            char *p1 = strstr(buf, "CONNECT\r\n");
            if (p1 != NULL)
            {
                p1 = p1 + strlen("CONNECT\r\n");
                char *p2 = strstr(p1, "\r\nOK\r\n");
                if (p2 != NULL)
                {
                    memcpy(response, p1, p2 - p1);
                    set_print_color(YELLOW);
                    printf("<http get>%.*s</http get>\n", (int)(p2 - p1), p1);
                    set_print_color(BLUE);
                    return 0;
                }
            }
        }
    }
    return -1;
}

#include <unistd.h>
#include <time.h>
int cat1_http_get(char *url, char *msg)
{
    int res;
    char buf[500] = {0};

    res = http_clear();
    if (res != 0)
        return -1;

    at_send(at_http_response_head_off);
    res = keyword_check("OK");
    if (res != 0)
        return -1;

    at_send(at_http_request_head_off);
    res = keyword_check("OK");
    if (res != 0)
        return -1;

    sprintf(buf, at_http_url, (int)strlen(url));
    at_send(buf);
    res = keyword_check("CONNECT");
    if (res != 0)
        return -1;

    raw_send(url, strlen(url));
    res = keyword_check("OK");
    if (res != 0)
        return -1;

    at_send(at_http_get_action);
    res = keyword_timeout_check("+QHTTPGET: 0,200,", 60);
    if (res != 0)
        return -1;

    //{"time":"20201125132110"}
    memset(buf, 0, sizeof(buf));
    at_send(at_http_read);
    res = http_read_check(buf);
    http_clear();
    if (res != 0)
    {
        return -1;
    }
    else
    {
        memcpy(msg, buf, strlen(buf));
        return 0;
    }
}

int cat1_http_get_time(char *url, struct tm *p_tm)
{
    int res;
    char buf[500] = {0};

    res = cat1_http_get(url, buf);
    if (res != 0)
    {
        printf("aaaaaaa1: res=%d\n", res);
        return -1;
    }

    char tm_str[20] = {0};
    res = sscanf(buf, "{\"time\":\"%[^\"]", tm_str);
    if (res != 1)
    {
        printf("aaaaaaa2: res=%d\n", res);
        return -1;
    }

    res = sscanf(tm_str, "%4d%2d%2d%2d%2d%2d", &p_tm->tm_year, &p_tm->tm_mon, &p_tm->tm_mday, &p_tm->tm_hour, &p_tm->tm_min, &p_tm->tm_sec);
    if (res != 6)
    {
        printf("aaaaaaa3: res=%d\n", res);
        return -1;
    }

    return 0;
}

int http_get_file(char *url, char *file_name, int *real_body_len)
{
    char buf[500] = {0};
    char head_buf[300] = {0};
    int size;
    int begin = 0;
    int end = 1024 * KB_N - 1; //2047
    int res = 0;
    int fail_cnt = 0;
    int response_body_size = 0;

    res = -1;
    // timeout_cnt = 0;
    /** 单次GET请求响应20sec超时*/
    int start_sec = get_second_sys_time();
    is_downloading = 1;

    cat1_tcp_close();
    sleep(3);
    cat1_mqtt_release(1);
    sleep(3);
    // suspend_all();

    while (1)
    {
        usleep(50000);

        if (get_second_sys_time() - start_sec > 20 * 60)
            goto ERR;

        if (fail_cnt >= 10)
        {
            yellow_log("err, http range get fail more than 6 times\n");
            is_downloading = 0;
            goto ERR;
        }

        http_clear();

        at_send(at_http_id);
        res = keyword_check("OK");
        if (res != 0)
            goto ERR;

        at_send(at_http_response_head_on);
        res = keyword_check("OK");
        if (res != 0)
            goto ERR;
        at_send(at_http_request_head_on);
        res = keyword_check("OK");
        if (res != 0)
            goto ERR;

        memset(buf, 0, sizeof(buf));
        sprintf(buf, at_http_url, strlen(url));
        at_send(buf);
        res = keyword_check("CONNECT");
        if (res != 0)
            goto ERR;
        raw_send(url, strlen(url));
        res = keyword_check("OK");
        if (res != 0)
            goto ERR;

        memset(head_buf, 0, sizeof(head_buf));
        res = http_range_head_create(head_buf, url, begin, end);
        if (res != 0)
            goto ERR;
        memset(buf, 0, sizeof(buf));
        sprintf(buf, at_http_user_get, strlen(head_buf));
        at_send(buf);
        res = keyword_check("CONNECT");
        if (res != 0)
            goto ERR;
        raw_send(head_buf, strlen(head_buf));
        response_body_size = http_res_206_len();
        if (response_body_size <= 0)
            goto ERR;

        at_send(at_http_read);
        res = http_read_file_check(&begin, &end, file_name, response_body_size);

        if (res == 0)
        {
            fail_cnt = 0;
            start_sec = get_second_sys_time();
            yellow_log("http range get once success.");
            continue;
        }
        else if (res == 1)
        {
            fail_cnt = 0;
            is_downloading = 0;
            // resume_all();
            return 0;
        }
        else
        {
            fail_cnt++;
            set_print_color(RED);
            printf("http get file once failed.try again ...\n");
            set_print_color(BLUE);
            continue;
        }
    }

ERR:
    cat1_fast_reboot();
    esp_restart();
    return -1;
}

int get_csq(void)
{
    at_send(at_csq);
    int res = csq_check();
    g_csq = res;
    return res;
}

int check_data_call_status(void)
{
    int res = -1;
    at_send(at_check_data_call);
    res = get_data_call_ip(NULL);
    if (res == 0)
    {
        g_data_call_ok = 1;
        return 0;
    }
    else
    {
        g_data_call_ok = 0;
        return -1;
    }
}

int data_call_clear(void)
{
    int res = -1;
    at_send(at_data_call_clear);
    res = keyword_check("OK");
    if (res != 0)
        return -1;
}

void cat1_get_oper_and_mode(char *oper, int *net_mode)
{
    at_send(at_net_info);
    net_info_check(oper, net_mode);
}

int cat1_set_greg(void)
{
    int res = -1;
    at_send(at_set_creg);
    res = keyword_check("OK");
    if (res != 0)
        return -1;
    return 0;
}

int cat1_get_lac_ci(uint8_t *lac, uint8_t *ci, int *lac_len, int *ci_len)
{
    int res = -1;
    at_send(at_creg);
    res = creg_check(lac, ci, lac_len, ci_len);
    return res;
}

int data_call(char *apn)
{
    char buf[1536] = {0};
    int res = -1;

    cat1_echo_disable();

    if (get_csq() < 12)
        return -1;

    at_send(at_cereg);
    int res1 = cereg_check();
    if (res1 == 0)
        g_cereg_ok = 1;
    else
    {
        g_cereg_ok = 0;
    }

    at_send(at_cgreg);
    res = cgreg_check();
    if (res == 0)
        g_cgreg_ok = 1;
    else
    {
        g_cgreg_ok = 0;
    }
    if (res1 != 0 && res != 0) //2G 4G both fail
        return -1;

    int net_mode = -1;
    at_send(at_net_info);
    res = net_info_check(NULL, &net_mode);
    if (res == 0)
        g_net_mode = net_mode;
    else
    {
        g_net_mode = -1;
    }

    if (res != 0 || net_mode < 0 || net_mode > 8) //4G: net_mode = 7
        return -1;

    at_send(at_check_data_call);
    res = get_data_call_ip(NULL);
    if (res == 0)
        return 0;

    /** clear data call*/
    at_send(at_data_call_clear);
    res = keyword_check("OK");
    if (res != 0)
        return -1;

    /** APN设置，拨号*/
    memset(buf, 0, sizeof(buf));
    sprintf(buf, at_set_apn, apn);
    at_send(buf);
    res = keyword_check("OK");
    if (res != 0)
        return -1;

    at_send(at_data_call);
    res = keyword_check("OK");
    if (res != 0)
        return -1;
    return 0;
}

int cat1_mqtt_release(int index)
{
    int res = -1;
    char buf[50] = {0};
    sprintf(buf, at_mqtt_disc, index);
    at_send(buf);
    res = keyword_check("+QMTDISC");

    memset(buf, 0, sizeof(buf));
    sprintf(buf, at_mqtt_close, index);
    at_send(buf);
    res = keyword_check("+QMTCLOSE");
    return res;
}

int cat1_echo_disable(void)
{
    at_send(at_disable_echo);
    return keyword_timeout_check("OK", 3);
}

int cat1_reboot(void)
{
    at_send(at_reboot);
    sleep(15);
    return keyword_check("\r\nRDY\r\n");
}

void cat1_fast_reboot(void)
{
    at_send(at_reboot);
    sleep(5);
}

int cat1_mqtt_conn(int index, char *host, int port, char *client_id, char *username, char *password)
{
    static int fail_cnt = 0;
    char buf[1536] = {0};
    int res = -1;

    cat1_echo_disable();
    res = cat1_mqtt_release(index);
    if (res != 0 && res != -1)
        return -1;

    res = cat1_cacaert_set(index);
    if (res != 0)
    {
        red_log("ca file set error: aswei");
        return -1;
    }

    memset(buf, 0, sizeof(buf));
    sprintf(buf, at_set_recv_mode, index);
    at_send(buf);
    res = keyword_check("OK");
    if (res != 0)
    {
        res = cat1_hard_reboot();
        if (res == 0)
        {
            int rslt = -1;
            while (1)
            {
                rslt = data_call("");
                if (rslt == 0)
                    break;
                sleep(1);
            }
        }
        if (res == 0 && get_second_sys_time() > 1800)
        {
            // netlog_write(88);
            // sleep(5);
            esp_restart();
            sleep(10);
        }
        return -1;
    }

    memset(buf, 0, sizeof(buf));
    sprintf(buf, at_set_mqtt_ver, index);
    at_send(buf);
    res = keyword_check("OK");
    if (res != 0)
        return -1;

    memset(buf, 0, sizeof(buf));
    sprintf(buf, at_set_ssl, index, 1, index); //ssl idx also use mqtt idx: 0~5

    at_send(buf);
    res = keyword_check("OK");
    if (res != 0)
        return -1;

    memset(buf, 0, sizeof(buf));
    sprintf(buf, at_set_ca_cert, index, ssl_name_tab[index]);
    at_send(buf);
    res = keyword_check("OK");
    if (res != 0)
        return -1;

    memset(buf, 0, sizeof(buf));
    sprintf(buf, at_set_ssl_level, index);
    at_send(buf);
    res = keyword_check("OK");
    if (res != 0)
        return -1;

    memset(buf, 0, sizeof(buf));
    sprintf(buf, at_ssl_ignore_valid, index);
    at_send(buf);
    res = keyword_check("OK");
    // if (res != 0)
    //     return -1;

    memset(buf, 0, sizeof(buf));
    sprintf(buf, at_set_ssl_version, index);
    at_send(buf);
    res = keyword_check("OK");
    if (res != 0)
        return -1;

    memset(buf, 0, sizeof(buf));
    sprintf(buf, at_set_ciphersuite, index);
    at_send(buf);
    res = keyword_check("OK");
    if (res != 0)
        return -1;

    memset(buf, 0, sizeof(buf));
    sprintf(buf, at_ssl_ignore_ltime, index);
    at_send(buf);
    res = keyword_check("OK");

    memset(buf, 0, sizeof(buf));
    sprintf(buf, at_ssl_ignore_chain, index);
    at_send(buf);
    res = keyword_check("OK");

    memset(buf, 0, sizeof(buf));
    sprintf(buf, at_set_keepalive, index);
    at_send(buf);
    res = keyword_check("OK");
    if (res != 0)
        return -1;

    memset(buf, 0, sizeof(buf));

    sprintf(buf, at_mqtt_open, index, host, port);

    printf("MQTT OPEN: %s\n", buf);
    at_send(buf);
    memset(buf, 0, sizeof(buf));
    sprintf(buf, "+QMTOPEN: %d,0", index);
    sleep(3);
    res = keyword_check(buf);
    if (res != 0)
        goto ERR;

    memset(buf, 0, sizeof(buf));
    sprintf(buf, at_mqtt_connect, index, client_id, username, password);
    printf("MQTT CONN: %s\n", buf);
    at_send(buf);
    memset(buf, 0, sizeof(buf));
    sprintf(buf, "+QMTCONN: %d,0,0", index);
    sleep(2);
    res = keyword_check(buf);
    if (res != 0)
        goto ERR;
    cat1_set_mqtt_ok(index);
    fail_cnt = 0;
    return 0;

ERR:
    // fail_cnt++;
    // if (fail_cnt > 3)
    // {
    //     cat1_set_data_call_error();
    //     fail_cnt = 0;
    // }
    cat1_set_mqtt_error(index);
    cat1_mqtt_release(index);
    return -1;
}

int cat1_mqtt_pub(int index, char *topic, char *payload)
{
    int res;
    static uint16_t msg_id = 0;
    static uint16_t res_msg_id = 0;

    // printf("[ pub ]%s,%s\n", topic, payload);
    char *buf = calloc(1, 256);
    sprintf(buf, at_mqtt_pub, index, ++msg_id, topic, (int)strlen(payload));
    at_send(buf);
    free(buf);
    res = keyword_check(">");
    if (res != 0)
    {
        cat1_set_mqtt_error(index);
        return -1;
    }

    raw_send(payload, strlen(payload));
    res = mqtt_pub_check(&res_msg_id);
    if (res == 0 && res_msg_id == msg_id)
    {
        set_print_color(YELLOW);
        printf("publish success: index=%d msgid=%hu\r\n", index, msg_id);
        set_print_color(BLUE);
        cat1_set_mqtt_ok(index);
        return 0;
    }
    else
    {
        cat1_set_mqtt_error(index);
        return -1;
    }
}

int cat1_mqtt_pub_qos_0(int index, char *topic, char *payload)
{
    int res;
    static uint16_t msg_id = 0;
    static uint16_t res_msg_id = 0;

    // printf("[ pub ]%s,%s\n", topic, payload);
    char *buf = calloc(1, 256);
    sprintf(buf, at_mqtt_pub_qos0, index, 0, topic, (int)strlen(payload));
    at_send(buf);
    free(buf);
    res = keyword_check(">");
    if (res != 0)
    {
        cat1_set_mqtt_error(index);
        return -1;
    }

    raw_send(payload, strlen(payload));
    res = mqtt_pub_check(&res_msg_id);
    if (res == 0 && res_msg_id == msg_id)
    {
        set_print_color(YELLOW);
        printf("publish success: index=%d msgid=%hu\r\n", index, msg_id);
        set_print_color(BLUE);
        cat1_set_mqtt_ok(index);
        return 0;
    }
    else
    {
        cat1_set_mqtt_error(index);
        return -1;
    }
}

int cat1_mqtt_sub(int index, char *topic)
{
    int res;

    char *buf = calloc(1, 1536);
    sprintf(buf, at_mqtt_sub, index, topic);
    at_send(buf);
    free(buf);
    res = mqtt_sub_check();
    if (res != 0)
    {
        cat1_set_mqtt_error(index);
        return -1;
    }
    cat1_set_mqtt_ok(index);
    return 0;
}

static void cat1_file_sum(char *str_sum, char *input, int input_len)
{
    int i = 0;
    uint8_t sum[2] = {0};
    if (input_len % 2 != 0)
    {
        sum[0] = input[input_len - 1];
    }

    for (i = 0; i < input_len / 2; i++)
    {
        sum[0] ^= (uint8_t)input[i * 2];
        sum[1] ^= (uint8_t)input[i * 2 + 1];
    }
    sprintf(str_sum, "%02x%02x", sum[0], sum[1]);
}

int cat1_upload_file(char *filename, char *filebody)
{
    char buf[1536] = {0};
    int res;

    sprintf(buf, at_read_ca_cert, filename);
    at_send(buf);

    char sum[50] = {0};
    char tag[50] = {0};
    cat1_file_sum(sum, filebody, strlen(filebody));
    sprintf(tag, "+QFDWL: %d,%s", (int)strlen(filebody), sum);
    char *s1[] = {tag, "+CME ERROR", "+QFDWL"};

    res = multi_keyword_check(3, s1, 4);
    if (res == 0)
    {
        set_print_color(YELLOW);
        printf("pem already exists:  %s **********************\n", filename);
        set_print_color(BLUE);
        return 0;
    }
    else
    {
        memset(buf, 0, sizeof(buf));
        sprintf(buf, at_del_ca_cert, filename);
        at_send(buf);
        char *s2[] = {"OK", "+CME ERROR"};
        multi_keyword_check(2, s2, 4);
    }

    memset(buf, 0, sizeof(buf));
    sprintf(buf, at_upload_ca_cert, filename, (int)strlen(filebody));
    at_send(buf);
    res = keyword_check("CONNECT");
    if (res != 0)
        return -1;

    raw_send(filebody, strlen(filebody));
    char upl_tag[50] = {0};
    sprintf(upl_tag, "+QFUPL: %d,%s", (int)strlen(filebody), sum);
    char *s3[] = {upl_tag, "+CME ERROR", "+QFUPL"};
    sleep(3); //response need some time
    res = multi_keyword_check(3, s3, 4);
    if (res != 0)
        return -1;

    set_print_color(YELLOW);
    printf("cacert set ok: %s **********************\n", filename);
    set_print_color(BLUE);
    return 0;
}

int cat1_cacaert_set(int index)
{
    return cat1_upload_file(ssl_name_tab[index], ssl_body_tab(index));
}

int cat1_upload_cli_crt(void)
{
    return cat1_upload_file(cli_name_tab[0], cli_body_tab(0));
}

int cat1_upload_cli_key(void)
{
    return cat1_upload_file(cli_name_tab[1], cli_body_tab(1));
}

void cat1_fresh(void)
{
    static int last = 0;
    int now = get_second_sys_time();
    if (now - last > 30)
    {
        last = now;
        get_csq();
    }
}
