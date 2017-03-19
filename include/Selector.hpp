//
// Created by discord on 16/11/16.
//

#ifndef MOTORDAEMON_SELECTOR_HPP
#define MOTORDAEMON_SELECTOR_HPP


#define CAMERA_SYSTEM_CALL_START "gst-launch-1.0 v4l2src device=/dev/video0 do-timestamp=true ! videoconvert ! videoscale ! video/x-raw,width=320,height=240 ! videorate ! video/x-raw,framerate=15/1 ! jpegenc quality=15 ! rtpjpegpay ! udpsink host="
#define CAMERA_SYSTEM_CALL_END " port=56988 &"
#define CAMERA_KILL_CALL "killall gst-launch-1.0"
#define INTERFACE "wlan0"

#include "../include/Odometry.hpp"
#include "../include/MotionController.hpp"
#include <sstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

#ifdef __arm__
Settings settings("/etc/MotorDaemon.conf");
#else
Settings settings("MotorDaemon.conf");
#endif

#ifdef __arm__
MotionController motion(settings);
#endif

std::vector<std::string> args = std::vector<std::string>();

static bool serverMode = false;
static bool proxyMode = false;


unsigned long Millisec(void)
{
    struct timeval tv;
    if(gettimeofday(&tv, NULL) != 0) return 0;
    return (tv.tv_sec * 1000ul) + (tv.tv_usec / 1000ul);
}

std::vector<std::string> split(std::string str, char delimiter)
{
    std::vector<std::string> internal;
    std::stringstream ss(str); // Turn the string into a stream.
    std::string tok;

    while(std::getline(ss, tok, delimiter)) {
        internal.push_back(tok);
    }

    return internal;
}

void getArgs(const std::string &s, char delim, std::vector<std::string> &elems)
{
    std::stringstream ss(s);
    ss.str(s);
    std::string item;

    while (std::getline(ss, item, delim))
    {
        elems.push_back(item);
    }
}


int treatOrder(std::string &order, std::function<void(char*)> print, bool proxyMode = false)
{
    if(!order.compare("exit")) return 1;

    args.clear();
    getArgs(order, ' ', args);

    if(args.size() == 0) return 0;

    else if(!args[0].compare("startcamera"))
    {
        system(CAMERA_KILL_CALL);

        if(args.size() != 2 && !proxyMode)
        {
            print((char *) "USAGE : startcamera <IP_client>\r\n");
            return 0;
        }

        std::string s;
        if(!proxyMode || args.size()==2)
        {
            s = std::string(CAMERA_SYSTEM_CALL_START)+std::string(args[1])+std::string(CAMERA_SYSTEM_CALL_END);
        } else {
            s = std::string(CAMERA_SYSTEM_CALL_START)+settings.get("IP_MOTORDAEMONPROXY")+std::string(CAMERA_SYSTEM_CALL_END);
        }

        system(s.c_str());
    }

    else if(!args[0].compare("stopcamera"))
    {
        system(CAMERA_KILL_CALL);
    }

    if(!args[0].compare("stop"))
    {
#ifdef __arm__
        //print("Ordering to stop\r\n");
        motion.stop();
#endif
        return 0;
    }
    else if(!args[0].compare("p"))
    {
#ifdef __arm__
        std::string s = std::to_string(motion.getX())+std::string(";")
                            +std::to_string(motion.getY())+std::string(";")
                            +std::to_string(motion.getAngle())+std::string("\r\n");
        print((char*)s.c_str());
#endif
        return 0;
    }
    else if(!args[0].compare("setksg"))
    {

        if(args.size() != 4)
        {
            print((char *) "USAGE : setksg <kp> <ki> <kd>\r\n");
            return 0;
        }

        float kp, ki, kd;
        try
        {
            kp = std::stof(args[1]);
            ki = std::stof(args[2]);
            kd = std::stof(args[3]);
        }
        catch (std::exception const &e)
        {
            print((char *) "BAD VALUE\r\n");
            return 0;
        }
#ifdef __arm__
        motion.setLeftSpeedTunings(kp, ki, kd);
#endif
        return 0;
    }

    else if(!args[0].compare("setksd"))
    {

        if(args.size() != 4)
        {
            print((char *) "USAGE : setksd <kp> <ki> <kd>\r\n");
            return 0;
        }

        float kp, ki, kd;
        try
        {
            kp = std::stof(args[1]);
            ki = std::stof(args[2]);
            kd = std::stof(args[3]);
        }
        catch (std::exception const &e)
        {
            print((char *) "BAD VALUE\r\n");
            return 0;
        }
#ifdef __arm__
        motion.setRightSpeedTunings(kp, ki, kd);
#endif
        return 0;
    }

    else if(!args[0].compare("setktd"))
    {

        if(args.size() != 4)
        {
            print((char *) "USAGE : setktd <kp> <ki> <kd>\r\n");
            return 0;
        }

        float kp, ki, kd;
        try
        {
            kp = std::stof(args[1]);
            ki = std::stof(args[2]);
            kd = std::stof(args[3]);
        }
        catch (std::exception const &e)
        {
            print((char*)"BAD VALUE\r\n");
            return 0;
        }
#ifdef __arm__
        motion.setTranslationTunings(kp, ki, kd);
#endif
        return 0;
    }

    else if(!args[0].compare("setkcd"))
    {

        if(args.size() != 4)
        {
            print((char *) "USAGE : setkcd <kp> <ki> <kd>\r\n");
            return 0;
        }

        float kp, ki, kd;
        try
        {
            kp = std::stof(args[1]);
            ki = std::stof(args[2]);
            kd = std::stof(args[3]);
        }
        catch (std::exception const &e)
        {
            print((char*)"BAD VALUE\r\n");
            return 0;
        }
#ifdef __arm__
        motion.setCurveTunings(kp, ki, kd);
#endif
        return 0;
    }

    else if(!args[0].compare("sweep"))
    {
        Servo s = Servo(1100000, 0, 1550000, 180);

        s.initPWM();

        for(int i= 0; i < 180 ; i+=5)
        {
            print((char*)("angle :" + std::to_string(i) + "\r\n").c_str());

            s.setAngle(i);
            usleep(1000*1000);
        }

        return 0;
    }

    else if(!args[0].compare("d"))
    {
        if(args.size() != 2)
        {
            print((char *) "USAGE : d <dist>\r\n");
            return 0;
        }

        long dist;
        try
        {
            dist = std::stol(args[1]);
        }
        catch (std::exception const &e)
        {
            print((char*)"BAD VALUE\r\n");
            return 0;
        }
#ifdef __arm__
        motion.orderTranslation(dist);
#endif
        return 0;
    }

    else if(!args[0].compare("followtraj"))
    {
        if(args.size() != 2)
        {
            print((char *) "USAGE : followtraj <dist1:curve1;dist2:curve2;...>\r\n");
            return 0;
        }


        std::vector<Cinematic> points = std::vector<Cinematic>();
        try
        {
            std::vector<std::string> strs = split(std::string(args[1]), ';');

            for(std::string &s : strs)
            {
                std::vector<std::string> sub = split(s, ':');
                Cinematic c(std::stod(sub[0]), std::stod(sub[1]));
                points.push_back(c);
            }
        }
        catch (std::exception const &e)
        {
            print((char*)"BAD VALUE\r\n");
            return 0;
        }

#ifdef __arm__
        motion.setTrajectory(points, points[points.size()-1].relativeDistance);
#endif
        return 0;
    }

    else if(!args[0].compare("go"))
    {

#ifdef __arm__
        motion.setControlled(false);
        motion.go();
#endif
        std::cout << "go received" << std::endl;
        return 0;
    }

    else if(!args[0].compare("setpos"))
    {
        if(args.size() != 3)
        {
            print((char *) "USAGE : setpos <x> <y>\r\n");
            return 0;
        }

        double x, y;
        try
        {
            x = std::stod(args[1]);
            y = std::stod(args[2]);
        }
        catch (std::exception const &e)
        {
            print((char*)"BAD VALUE\r\n");
            return 0;
        }
#ifdef __arm__
        motion.setPosition(x,y);
#endif
        return 0;
    }

    else if(!args[0].compare("setang"))
    {
        if(args.size() != 2)
        {
            print((char *) "USAGE : setang <rad>\r\n");
            return 0;
        }

        double o;
        try
        {
            o = std::stod(args[1]);
        }
        catch (std::exception const &e)
        {
            print((char*)"BAD VALUE\r\n");
            return 0;
        }
#ifdef __arm__
        motion.setAngle(o);
#endif
        return 0;
    }

    else if(!args[0].compare("gor"))
    {

#ifdef __arm__
        motion.setControlled(false);
        motion.goR();
#endif
        std::cout << "gor received" << std::endl;
        return 0;
    }

    else if(!args[0].compare("sweepR"))
    {

#ifdef __arm__
        motion.sweep(false);
#endif
        std::cout << "sweepR received" << std::endl;
        return 0;
    }

    else if(!args[0].compare("sweepL"))
    {

#ifdef __arm__
        motion.sweep(true);
#endif
        std::cout << "sweepL received" << std::endl;
        return 0;
    }

    else if(!args[0].compare("sweepstop"))
    {

#ifdef __arm__
        motion.stopSweep();
#endif
        return 0;
    }

    else if(!args[0].compare("cr"))
    {
        if(args.size() != 2)
        {
            print((char *) "USAGE : cr <rad>\r\n");
            return 0;
        }

        long dist;
        try
        {
            dist = std::stol(args[1]);
        }
        catch (std::exception const &e)
        {
            print((char*)"BAD VALUE\r\n");
            return 0;
        }
#ifdef __arm__
        motion.orderCurveRadius(dist);
#endif
        return 0;
    }

    else if(!args[0].compare("seta"))
    {
        if(args.size() != 2)
        {
            print((char *) "USAGE : seta <angle in radians>\r\n");
            return 0;
        }

        float angle;
        try
        {
            angle = std::stof(args[1]);
        }
        catch (std::exception const &e)
        {
            print((char*)"BAD VALUE\r\n");
            return 0;
        }
#ifdef __arm__
        motion.orderAngle(angle);
#endif
        return 0;
    }

    else if(!args[0].compare("sets"))
    {
        if(args.size() != 2)
        {
            print((char *) "USAGE : sets <speed in ticks/s>\r\n");
            return 0;
        }

        int s;
        try
        {
            s = std::stoi(args[1]);
        }
        catch (std::exception const &e)
        {
            print((char*)"BAD VALUE\r\n");
            return 0;
        }
#ifdef __arm__
        motion.setSpeedTranslation(s);
#endif
        return 0;
    }

    else if(!args[0].compare("ts"))
    {

        if(args.size() != 2)
        {
            print((char *) "USAGE : ts <speed>\r\n");
            return 0;
        }

        long dist;
        try
        {
            dist = std::stol(args[1]);
        }
        catch (std::exception const &e)
        {
            print((char*)"BAD VALUE\r\n");
            return 0;
        }

#ifdef __arm__
        motion.testSpeed(dist);
#endif

        return 0;
    }

    else if(!args[0].compare("c"))
    {
#ifdef __arm__
        print((char*)(
                std::to_string(motion.getOdometry()->getLeftValue())+std::string(" ; ")+std::to_string(motion.getOdometry()->getRightValue())+std::string("\r\n")+
                std::to_string(motion.getCurveRadius())+std::string("\r\n\n")).c_str());
#endif
        return 0;
    }

    else if(!args[0].compare("k"))
    {
#ifdef __arm__
        print((char*) motion.getTunings());
#endif
        return 0;
    }

    else if(!args[0].compare("m"))
    {
#ifdef __arm__
        print((char*) motion.isMoving());
#endif
        return 0;
    }

    else if(!args[0].compare("sv"))
    {
#ifdef __arm__
        std::string s = std::to_string(Millisec())+std::string(";")+std::to_string(motion.getCSpeedL())+std::string(";")+std::to_string(motion.getSpeedL())+
            std::string(";")+std::to_string(motion.getCSpeedR())+std::string(";")+std::to_string(motion.getSpeedR())+std::string("\r\n");
        print((char*)s.c_str());
#endif
        return 0;
    }

    else
    {
        print((char *) "No such command\r\n");
    }

    return 0;

}

#endif //MOTORDAEMON_SELECTOR_HPP
