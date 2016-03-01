// This library is a simple OpenCV parameter controller using midi physical fader controller via control change
#include <windows.h>
#include <mmsystem.h>
#include <map>
#include <list>
#include <mutex>
#include <algorithm>
//===========================
// data struct definitions
//===========================

// Holds midi in device
struct midiin_dev {
	int devid;
	std::string devname;
};

// MIDI button status
struct button_status {
	DWORD status; // control change status and channel
	DWORD contnum; // control change number
	char val; // usually 7fh=push 0h=release
};

// Data pool, users need not touch
static std::mutex midiin_cv_mtx;
static std::map<DWORD,char> latest_volfader_vals;
static std::vector<DWORD> registered_buttons;
static std::list<button_status> latest_button_queue;

// MIDI receive call back definition
void CALLBACK midiin_cv_callback(HMIDIIN hMidiIn, UINT wMsg, DWORD dwInstance,
	DWORD dwParam1, DWORD dwParam2);

// Library main body
class midi_cv_ctrl {
public:
	// Constructor
	midi_cv_ctrl() {
		devid = 0;
		error_no = 0;
		errmsg[0] = '\0';
		is_open = false;
	}
	
	// Retrieve midi in devices before open.
	bool midiin_devlist(std::vector<midiin_dev> &devlist,int &devnum) {
		UINT id; 
		MIDIINCAPS inCaps;
		midiin_dev dl;
		
		devlist.clear();
		devnum = midiInGetNumDevs();

		for (id=0; id<devnum; id++) {
			res = midiInGetDevCaps(id, &inCaps, sizeof(inCaps));
			if (res != MMSYSERR_NOERROR) { continue; }
			dl.devid = id;
			dl.devname = std::string(inCaps.szPname);
			devlist.push_back(dl);
		}
		return true;
	}
	
	// Opens a midi in device.
	bool midiin_open(int pDev) {
		res = midiInOpen(&hMidiIn, pDev,
			(DWORD_PTR)midiin_cv_callback, 0, CALLBACK_FUNCTION);
		
		if (res != MMSYSERR_NOERROR) {
			midiInGetErrorText(res, errmsg, sizeof(errmsg));
			return false;
		}
		is_open = true;
		latest_volfader_vals.clear();
		latest_button_queue.clear();
		registered_buttons.clear();
		return true;
	}
	
	// Start midi in
	bool midiin_start() {
		if(!is_open)
			return false;
		midiInStart(hMidiIn);
		return true;
	}
	
	// Stop midi in
	bool midiin_stop() {
		if(!is_open)
			return false;
		midiInStop(hMidiIn);
		return true;
	}
	
	// Reset midi in device
	bool midiin_reset() {
		if(!is_open)
			return false;
		midiInReset(hMidiIn);
		latest_volfader_vals.clear();
		latest_button_queue.clear();
		registered_buttons.clear();
		return true;
	}
	
	// Close midi in device
	bool midiin_close() {
		if(!is_open)
			return false;
		midiInClose(hMidiIn);
		is_open = false;
		latest_volfader_vals.clear();
		latest_button_queue.clear();
		registered_buttons.clear();
		return true;
	}
	
	// Get error number and message, but does not work so much for now.
	int get_error_no() {
		return error_no;
	}
	std::string get_errmsg() {
		return (std::string(errmsg));
	}
	
	// Register fader or volume controller(s) to watch. usually pStatus=0xbX X=midi ch, pContNum=control change number
	bool register_volfader(DWORD pStatus,DWORD pContNum) {
		DWORD val;
		mutex_lock();
		
		val = pStatus | (pContNum << 8);
		
		std::map<DWORD,char>::iterator it;
 
        it = latest_volfader_vals.find(val);
 
        if (it == latest_volfader_vals.end() ) {
			latest_volfader_vals[val] = -1; // default value is set to -1 temporary, since current actual value is unknown until first midi message is received.
		} else { // already exist
			mutex_unlock();
			return false;
		}
		mutex_unlock();
		
		return true;
	}
	
	// Get current values of fader or volume controller(s)
	bool get_volfader_val(DWORD pStatus,DWORD pContNum,char &pVal) {
		DWORD msg;
		
		mutex_lock();
		
		msg = pStatus | (pContNum << 8);
		
		std::map<DWORD,char>::iterator it;
 
        it = latest_volfader_vals.find(msg);
 
        if (it == latest_volfader_vals.end() ) { // not found
			mutex_unlock();
			return false;
		} else { // already exist
			pVal = it->second;
		}
		
		mutex_unlock();
		return true;
	}
	
	// Used by internal callback function, user need not touch.
	static bool update_volfader_val(DWORD pMsg) {
		mutex_lock();
		
		std::map<DWORD,char>::iterator it;
 
        it = latest_volfader_vals.find(pMsg & 0xffff);
 
        if (it == latest_volfader_vals.end() ) { 
			mutex_unlock();
			return false;
		} else { // already exist
			it->second = (char)(pMsg >> 16);
		}
		
		mutex_unlock();
		return true;
	}
	
	// Register button controller(s) to watch. usually pStatus=0xbX X=midi ch, pContNum=control change number
	bool register_button(DWORD pStatus,DWORD pContNum) {
		DWORD val;
		
		if(find_registered_button(pStatus,pContNum)) {
			return false;
		}
			
		mutex_lock();
		val = pStatus | (pContNum << 8);
		registered_buttons.push_back(val);
 		mutex_unlock();
		return true;
	}
	
	// Get updated button operation history in std::list format. The history will be erased after this func is called.
	bool get_button_queue(std::list<button_status> &pBs) {
		mutex_lock();
		std::copy(latest_button_queue.begin(),latest_button_queue.end(),back_inserter(pBs));
		latest_button_queue.clear();
		mutex_unlock();
		return true;
	}
	
	// Used by internal callback function, user need not touch.
	static bool update_button_queue(DWORD pMsg) {
		DWORD status = pMsg & 0xff;
		DWORD contnum = (pMsg >> 8) & 0xff;
		DWORD val = (pMsg >> 16) & 0xff;
		button_status bs;
		
		if(!find_registered_button(status,contnum)) {
			return false;
		}
		
		bs.status = status;
		bs.contnum = contnum;
		bs.val = (char)val;
		
		mutex_lock();
		latest_button_queue.push_back(bs);
		mutex_unlock();
		return true;
	}

	// Used internall , user need not touch.
	static void mutex_lock() {
		midiin_cv_mtx.lock();
	}
	static void mutex_unlock() {
		midiin_cv_mtx.unlock();
	}
	
protected:
	// Used internall , user need not touch.
	static bool find_registered_button(DWORD pStatus,DWORD pContNum) {
		DWORD val;
		mutex_lock();
		
		val = pStatus | (pContNum << 8);
		
		std::vector<DWORD>::iterator it = std::find(registered_buttons.begin()
			,registered_buttons.end()
			,val );
 		
 		mutex_unlock();
        if (it == registered_buttons.end() ) {
			return false;
		}  
		
		return true;
	}
	
	HMIDIIN hMidiIn; 
	MMRESULT res; 
	UINT devid; 
	char errmsg[MAXERRORLENGTH];
	int error_no;
	bool is_open;
};

// Midi In callback function, user need not touch.
void CALLBACK midiin_cv_callback(HMIDIIN hMidiIn, UINT wMsg, DWORD dwInstance,
	DWORD dwParam1, DWORD dwParam2) {
	midi_cv_ctrl::update_volfader_val(dwParam1);
	midi_cv_ctrl::update_button_queue(dwParam1);
}
