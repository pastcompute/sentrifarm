# Frequently Asked Questions

Q. You have had four months, and you havent really done all that much!
A. We all work full time and have young families. Also farming, the weather doesn't wait for you... 
We actually expect to make more progress in the warmer months, especially with extending the range of the radio links, but that is after the competition (it is winter down here in August...)

Q. Why have you used all these different compute modules?
A. Quick answer: time and money.

(a) When I started I had an unused carambola2 and it used less power than a raspberry pi.
(b) When then figured it didnt matter if the farm station was a Pi because it had ready access to power
(c) We wanted at least one module running Linux because that gives us a lot of future options for quick and easy raw processing power and USB connections
(d) I had recent experience using the ESP8266, it is very cheap and has a deep sleep mode. And on-site wifi...
(e) I won a teensy early in the competition :-) and the teensy doesnt need an external ADC
(f) Pi's are relatively easy to obtain in australia and cheap, so we can use them for sensor prototyping whilst the other modules are out in a paddock somewhere

Q. Whats with the circuit boards?
A. When I started I only had the carambola2, and we didnt know what sensors we would use, so I built a shield that takes the inAir9 plus a whole bunci of I/O support.  Then when we realised it would be useful to support a wide variety of computers, rather than waste the carambola2 shields I designed the adaptor board instead. Plus I was trying to maximise the benefit of the OSHPark credit I won from hackaday :-) Also, we only need to buy two radios to start with, and we can move them around between configurations for testing

Q. Why no GPS?
A. At this stage we will make manual note of the position when each node is deployed, that way we dont need to have the power to run GPS. Plus, costs more money.

Q. You mention a smoke detector, how will that work?
A. We dont know, the MQ2 claims to be a smoke detector but presume it works 'differently' to a typical 'radioactive' household smoke detector.  That is part of the whole project, to find these things out.

Q. Why only point to point links?
A. Time and money, but mostly time.
The radios are not that expensive ($20) but I dont want to buy a whole bunch until after doing a lot more trialling with the pair we have.  The time is a bigger factor, because I need to write code to implement the LoRa specification, the xisting reference implementions are not suitable for how I want to evolve the software.
