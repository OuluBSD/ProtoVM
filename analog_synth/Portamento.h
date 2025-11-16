#ifndef PORTAMENTO_H
#define PORTAMENTO_H

class Portamento {
public:
    Portamento();

    void setTime(double time);  // Time in seconds to glide from one note to another
    void setEnabled(bool enabled);

    void setTargetFrequency(double freq);
    void setCurrentFrequency(double freq);

    double getNextFrequency();  // Returns interpolated frequency

    bool isAtTarget() const { return currentFreq == targetFreq; }
    
    // Getter methods for preset system
    double getTime() const { return time; }
    bool isEnabled() const { return enabled; }
    double getTargetFrequency() const { return targetFreq; }
    double getCurrentFrequency() const { return currentFreq; }

private:
    double time;        // Glide time in seconds
    bool enabled;
    double targetFreq;
    double currentFreq;
    double stepSize;    // How much to change per sample
    bool active;

    static const int sampleRate = 44100;
};

#endif // PORTAMENTO_H