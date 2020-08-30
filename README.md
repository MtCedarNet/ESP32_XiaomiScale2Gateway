# ESP32_XiaomiScale2Gateway
An Arduino sketch to send Xiaomi Scalse2 data to the IFTTT Webhook with ESP32.

* ServiceData format

0: unknown
1: status bits: [Memory] 0 [Fixed] 0  0 1 [Impedance] 0
2-3: year
4: month
5: day
6: hour
7: munite
8: second
9-10: impedance
11-12: weight (5g)

referred to
https://dev.to/henrylim96/reading-xiaomi-mi-scale-data-with-web-bluetooth-scanning-api-1mb9
