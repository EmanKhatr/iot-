import serial
import speech_recognition as sr

# Initialize serial communication
ser = serial.Serial('COM3', 9600)

recognizer = sr.Recognizer()

try:
    while True:
        try:
            # Capture audio from microphone
            with sr.Microphone() as source:
                print("Say something...")
                audio = recognizer.listen(source, phrase_time_limit=2)

            # Convert speech to text
            print("Processing voice...")
            text = recognizer.recognize_google(audio, language='ar-EG')
            print("You said:", text)

            # Commands
            if text == "نور اللمبه":
                ser.write(b'1')
                print("LED ON")

            elif text == "اطفي اللمبه":
                ser.write(b'0')
                print("LED OFF")

        except sr.UnknownValueError:
            print("Could not understand audio")

        except sr.RequestError:
            print("API error")

        finally:
            input("\nPress Enter to continue...")

except:
    ser.close()
    print("Serial connection closed.")