#include<opencv/cv.h>

class ImageHandler
{
public:
        enum Tool {fill, polygon};

/* vars */
private:
        IplImage* _img;
        IplImage* _mask;
        IplImage* _window;
	IplImage* _small;
	IplImage* _previous;

        Tool _curTool;
	char* _filename;

        int _sqFillThreshold;
	int _polygonClickCt;
	CvPoint *_polygonClick;

public:
/* Constructor / Destructor */
	ImageHandler();
	~ImageHandler();

/* Functions */
        void loadFile(char* filename);
        void clicked(int x, int y, int maxx, int maxy);
	void undo();  // Not implemented yet.
	void save();
	void switchTool(); 
        IplImage* getWindow();
	void setWindowSize(int x, int y);
	void changeThreshold(int diff);

protected:
	void calcWindow();
        void init();
	void saveUndo();

/* Tools */
	void polygonTool(int x, int y);
	void fillTool(int x, int y, CvScalar* seedColor);

};

