//
//  ofxGCode
//
//  Created by Andrew Wallace on 11/7/19.
//

#include "ofxGCode.hpp"

void ofxGCode::setup(ofRectangle usableCanvas){
    //set some defaults
    circle_resolution = 50;

    //inches for axidraw
    pixels_per_inch = 1;

    //set_size(ofGetWidth(), ofGetHeight());
	clip.setup(usableCanvas.getTopLeft(), usableCanvas.getBottomRight());
	this->usableCanvas = usableCanvas;

    //display stuff
    show_transit_lines = false;
    show_path_with_color = false;
    show_do_not_reverse = false;
    demo_col.set(0,0,0);
    demo_fade_prc = 0.75;
}

//void ofxGCode::set_size(int w, int h){
//    clip.setup(ofVec2f(0, 0), ofVec2f(w,h));
//    
//    clear();
//}

void ofxGCode::clear(){
    lines.clear();
	usedCanvas = ofRectangle();
}

void ofxGCode::draw(int max_lines_to_show){

	float sx = ofGetWidth() / usableCanvas.getRight();
	float sy = ofGetHeight() / usableCanvas.getBottom();
	float scale = std::min(sx, sy);

	ofPushMatrix();
	ofScale(scale, scale);
	ofTranslate(-usableCanvas.getLeft()/2, -usableCanvas.getTop()/2);

	//draw canvas
	ofSetColor(0, 10);
	ofDrawRectangle(usableCanvas);
	ofSetColor(255);

    int draw_count = 0;
    if (max_lines_to_show <= 0) max_lines_to_show = lines.size();

	ofVboMesh linesMesh;
	ofVboMesh movesMesh;
	linesMesh.setMode(OF_PRIMITIVE_LINES);
	movesMesh.setMode(OF_PRIMITIVE_LINES);

    int end_index = MIN(max_lines_to_show, lines.size());
    for (int i=0; i<end_index; i++){
        //the line
        GLine & line = lines[i];
        
        //ofSetColor(demo_col.r, demo_col.g, demo_col.b, 255 * demo_fade_prc);
        
        //fading between colors to show order
//        if (show_path_with_color){
//            float prc = (float)i/(float)lines.size();
//            ofSetColor(0, 255.0*(1.0-prc), 255*prc);
//        }
        
//        if (show_do_not_reverse && line.do_not_reverse){
//            ofSetColor(255, 38, 226);
//            //throw wings on it
//            float prc = 0.9;
//            ofVec2f wing_pnt;
//            wing_pnt.x = (1.0-prc)*line.a.x + prc*line.b.x;
//            wing_pnt.y = (1.0-prc)*line.a.y + prc*line.b.y;
//            //ofDrawCircle(wing_pnt.x, wing_pnt.y, 2);
//            float angle = atan2(line.a.y-line.b.y, line.a.x-line.b.x);
//            float dist = 7;
//            float spread = PI/8;
//            ofDrawLine(wing_pnt.x, wing_pnt.y, wing_pnt.x+cos(angle+spread)*dist, wing_pnt.y+sin(angle+spread)*dist);
//            ofDrawLine(wing_pnt.x, wing_pnt.y, wing_pnt.x+cos(angle-spread)*dist, wing_pnt.y+sin(angle-spread)*dist);
//        }
        
        //line.draw();
		linesMesh.addVertex(glm::vec3(line.a.x,line.a.y, 0));
		linesMesh.addVertex(glm::vec3(line.b.x,line.b.y, 0));

        
        //the transit to the next line
        if (show_transit_lines && i < end_index-1){
            ofVec2f & transit_start = line.b;
            ofVec2f & transit_end = lines[i+1].a;
            if (transit_start != transit_end){
                //ofSetColor(255, 0,0, 60);
                //ofDrawLine(transit_start, transit_end);
				movesMesh.addVertex(glm::vec3(transit_start.x,transit_start.y, 0));
				movesMesh.addVertex(glm::vec3(transit_end.x,transit_end.y, 0));
            }
        }
    }

	ofSetColor(0,0,0,255);
	linesMesh.draw();

	if(show_transit_lines){
		ofSetColor(255,0,0,40);
		movesMesh.draw();
		ofSetColor(255);
	}

	ofNoFill();
		ofSetColor(255,128,0);
		ofDrawRectangle(usedCanvas);
		ofSetColor(0,255,255);
		ofDrawRectangle(usableCanvas);
	ofFill();

	ofSetColor(255);
	ofPopMatrix();
}

float ofxGCode::flipX(float &x) const{
	return ofMap(x, usableCanvas.getLeft(), usableCanvas.getRight(), usableCanvas.getRight(), usableCanvas.getLeft(), true);
}

float ofxGCode::flipY(float &y) const{
	return ofMap(y, usableCanvas.getTop(), usableCanvas.getBottom(), usableCanvas.getBottom(), usableCanvas.getTop(), true);
}

//genertaes gcode and writes it to a file
void ofxGCode::save(string name){
    float inches_per_pixel = 1.0 / pixels_per_inch;
    
    vector<string> commands;
    commands.clear();
    
    //pen up and positioned at the origin
    //commands.push_back("M3 S0");
    //commands.push_back("G0 X0 Y0");

	commands.push_back("G28"); //autohome
	commands.push_back("G0 X0 Y0 Z" + ofToString(pen_down_value + liftPenMm * 3) + " F2000"); //lift pen
	commands.push_back("G0 X0 Y35 Z" + ofToString(pen_down_value + liftPenMm * 3) + " F2000"); //avoid clip
	//move to our "origin" (clip box)
	//commands.push_back("G0 X" + ofToString(clip.min.x,2) + " Y"  + ofToString(clip.min.y,2) + " Z" + ofToString(pen_down_value + liftPenMm) );
    
    ofVec2f last_pos = clip.min;

	string extraSpeedCommand;
	if(moveSpeed > 0){
		extraSpeedCommand = " F" + ofToString(moveSpeed);
	}

	string penUp = " Z" + ofToString(pen_down_value + liftPenMm);
	string penDown = " Z" + ofToString(pen_down_value);

    for(int i=0; i<lines.size(); i++){
        GLine & line = lines[i];
        ofVec2f pos_a = ofVec2f((line.a.x) * inches_per_pixel, flipY(line.a.y) * inches_per_pixel);
        ofVec2f pos_b = ofVec2f((line.b.x) * inches_per_pixel, flipY(line.b.y) * inches_per_pixel);

		float dist = pos_a.distance(last_pos);
        //if we are not at the start point, pen up and move there and pen down
        if ( dist > 0.01){ //next line is not a continuous one, lift pen, move to the next line

			//pen UP in the last pos we moved to
			commands.push_back("G0 X" + ofToString(last_pos.x,2) + " Y"  + ofToString(last_pos.y,2) + penUp + " F2000");

			//move to the new pos
            commands.push_back("G0 X"+ofToString(pos_a.x,2) + " Y" + ofToString(pos_a.y,2) + penUp + " F2000"); //move pen (while up) at top speed

			//pen DOWN
			commands.push_back("G0 X"+ofToString(pos_a.x,2) + " Y" + ofToString(pos_a.y,2) + penDown + " F2000");

        }else{
            //cout<<"do not life pen at "<<line.a<<endl;
        }
        
        //move to the end point
        commands.push_back("G1 X" + ofToString(pos_b.x) + " Y" + ofToString(pos_b.y) + penDown + extraSpeedCommand);
        
        //store it
        last_pos = pos_b;
    }
    
    //add some closing steps
	commands.push_back("G0 X" + ofToString(last_pos.x,2) + " Y"  + ofToString(last_pos.y,2) + penUp + extraSpeedCommand );
	commands.push_back("G0 X0 Y225 " + penUp + extraSpeedCommand);
    
	ofLogNotice() << "transit distance: " << measureTransitDistance();
    
    //write it to file
	ofLogNotice() << "saving " << commands.size() << " commands";
    ofFile myTextFile;
    myTextFile.open(name,ofFile::WriteOnly);
    for (int i=0; i<commands.size(); i++){
        myTextFile<<commands[i]<<endl;
    }
    
    ofLogNotice() << "SAVED";
}


void ofxGCode::rect(ofRectangle box){
    rect(box.x, box.y, box.width, box.height);
}
void ofxGCode::rect(float x, float y, float w, float h){
    line(x,y, x+w, y);
    line(x+w,y, x+w, y+h);
    line(x+w,y+h, x, y+h);
    line(x,y+h, x, y);
}

void ofxGCode::rounded_rect(ofRectangle rect, float corner_size, int corner_resolution){
    vector<ofVec2f> pnts = get_rounded_pnts(rect.x, rect.y, rect.width, rect.height, corner_size, corner_resolution);
    polygon(pnts);
}
void ofxGCode::rounded_rect(float x, float y, float w, float h, float corner_size, int corner_resolution){
    vector<ofVec2f> pnts = get_rounded_pnts(x, y, w, h, corner_size, corner_resolution);
    polygon(pnts);
}

vector<ofVec2f> ofxGCode::get_rounded_pnts(ofRectangle rect, float corner_size, int corner_resolution){
    return get_rounded_pnts(rect.x, rect.y, rect.width, rect.height, corner_size, corner_resolution);
}
vector<ofVec2f> ofxGCode::get_rounded_pnts(float x, float y, float w, float h, float corner_size, int corner_resolution){
    
    ofRectangle base = ofRectangle(x,y,w,h);
    
    vector<ofVec2f> pnts;
    
    pnts.push_back( ofVec2f(base.x+corner_size, base.y) );
    pnts.push_back( ofVec2f(base.x+base.width-corner_size, base.y) );
    
    //top right
    for (int i=1; i<corner_resolution; i++){
        ofVec2f center;
        center.x = base.x+base.width-corner_size;
        center.y = base.y+corner_size;
        
        float angle = ofMap(i, 0, corner_resolution, -PI/2, 0);
        ofVec2f pos;
        pos.x = center.x + cos(angle) * corner_size;
        pos.y = center.y + sin(angle) * corner_size;
        
        pnts.push_back(pos);
    }
    
    pnts.push_back( ofVec2f(base.x+base.width, base.y+base.height-corner_size) );
    
    //bottom right
    for (int i=1; i<corner_resolution; i++){
        ofVec2f center;
        center.x = base.x+base.width-corner_size;
        center.y = base.y+base.height-corner_size;
        
        float angle = ofMap(i, 0, corner_resolution, 0, PI/2);
        ofVec2f pos;
        pos.x = center.x + cos(angle) * corner_size;
        pos.y = center.y + sin(angle) * corner_size;
        
        pnts.push_back(pos);
    }
    
    pnts.push_back( ofVec2f(base.x+corner_size, base.y+base.height) );
    
    //bottom left
    for (int i=1; i<corner_resolution; i++){
        ofVec2f center;
        center.x = base.x+corner_size;
        center.y = base.y+base.height-corner_size;
        
        float angle = ofMap(i, 0, corner_resolution, PI/2, PI);
        ofVec2f pos;
        pos.x = center.x + cos(angle) * corner_size;
        pos.y = center.y + sin(angle) * corner_size;
        
        pnts.push_back(pos);
    }
    
    pnts.push_back( ofVec2f(base.x, base.y+corner_size) );
    
    //top left
    for (int i=1; i<corner_resolution; i++){
        ofVec2f center;
        center.x = base.x+corner_size;
        center.y = base.y+corner_size;
        
        float angle = ofMap(i, 0, corner_resolution, PI, (3*PI)/2);
        ofVec2f pos;
        pos.x = center.x + cos(angle) * corner_size;
        pos.y = center.y + sin(angle) * corner_size;
        
        pnts.push_back(pos);
    }
    
    return pnts;
}

void ofxGCode::circle(ofVec2f center, float size){
    circle(center.x, center.y, size);
}
void ofxGCode::circle(float x, float y, float size){
    float angle_step =(TWO_PI/(float)circle_resolution);
    begin_shape();
    for (int i=0; i<circle_resolution; i++){
        ofVec2f pnt;
        float angle = angle_step  * i;
        
        pnt.x = x + sin(angle) * size;
        pnt.y = y + cos(angle) * size;
        vertex(pnt.x, pnt.y);
    }
    end_shape(true);
}
vector<ofVec2f> ofxGCode::get_circle_pnts(ofVec2f center, float size, int steps){
    float angle_step = TWO_PI/steps;
    vector<ofVec2f> pnts;
    for (int i=0; i<steps; i++){
        float angle = angle_step * i;
        ofVec2f pos;
        pos.x = center.x + cos(angle) * size;
        pos.y = center.y + sin(angle) * size;
        pnts.push_back(pos);
    }
    return pnts;
}

void ofxGCode::spiral(float x, float y, float size, float turnsDensity){
	float angle_step =(TWO_PI/(float)circle_resolution);
	begin_shape();

	//float area = M_PI * size * size;
	size_t n = circle_resolution * size * turnsDensity;
	for (size_t i=0; i < n; i++){
		ofVec2f pnt;
		float angle = angle_step  * i;
		float pctAndSize = size * (i) / double(n - 1);
		pnt.x = x + sinf(angle) * pctAndSize;
		pnt.y = y + cosf(angle) * pctAndSize;
		vertex(pnt.x, pnt.y);
	}
	end_shape(false);
}

void ofxGCode::circleFill(float x, float y, float size, float turnsDensity){
	float angle_step =(TWO_PI/(float)circle_resolution);
	begin_shape();
	//(x)^2 + (y)^2 = r^2
	// x = sqr(r^2 - y^2)
	size_t n = size * turnsDensity;
	for (size_t i=0; i < n; i++){
		float pct = (i) / double(n - 1);
		float yy = size * pct;
		float xx = sqrtf( size * size - yy * yy);
		float angle = angle_step  * i;
		ofVec2f pnt;
		pnt.x = x + xx;
		pnt.y = y + yy;
		vertex(pnt.x, pnt.y);
		pnt.x = x - xx;
		vertex(pnt.x, pnt.y);
	}
	end_shape(false);
}

void ofxGCode::udpateOfMatrix(){
	//update the OF matrix
	ofMatrix4x4 mat = ofGetCurrentMatrix(OF_MATRIX_MODELVIEW);
	ofMatrix = mat.getTransposedOf(mat);
}

//Emulating the begin/end shape functionality
void ofxGCode::begin_shape(bool updateOfMatrix){
	if(updateOfMatrix){
		udpateOfMatrix();
	}
    shape_pnts.clear();
}
void ofxGCode::vertex(const ofVec2f & p){
    shape_pnts.push_back(p);
}
void ofxGCode::vertex(const float & x, const float & y){
    shape_pnts.push_back(ofVec2f(x,y));
}
void ofxGCode::end_shape(bool close){
    if (shape_pnts.size() < 2){
        //cout<<"not enough points to make a shape"<<endl;
        return;
    }
    for (int i=0; i<shape_pnts.size()-1; i++){
        line(shape_pnts[i].x, shape_pnts[i].y, shape_pnts[i+1].x, shape_pnts[i+1].y);
    }
    if (close){
        line(shape_pnts[shape_pnts.size()-1].x, shape_pnts[shape_pnts.size()-1].y, shape_pnts[0].x, shape_pnts[0].y);
    }
}

//drawing polygone from points
void ofxGCode::polygon(vector<ofVec2f> pnts, bool close_shape){
    begin_shape();
    for (int i=0; i<pnts.size(); i++){
        vertex(pnts[i]);
    }
    end_shape(close_shape);
}

//Lines
void ofxGCode::line(GLine & _line){
    if (_line.skip_me)  return;
    line(_line.a.x,_line.a.y, _line.b.x,_line.b.y);
}
void ofxGCode::line(ofVec2f & a, ofVec2f & b){
    line(a.x, a.y, b.x, b.y);
}

void ofxGCode::line(float x1, float y1, float x2, float y2){
	ofVec2f p1 = getModelPoint(x1,y1);
	ofVec2f p2 = getModelPoint(x2,y2);
    
    //clip the points to fit our canvas, rejecting the line if it would be entirely out of bounds
    if (!clip.clip(p1, p2)) {
        //cout<<"no part of this line is on screen"<<endl;
        return;
    }
    
    GLine line;
    line.set(p1, p2);
    lines.push_back(line);
}

//adds a vector of GLines
void ofxGCode::add_lines(vector<GLine> & new_lines){
    for (int i=0; i<new_lines.size(); i++){
        line(new_lines[i]);
    }
}

//Thick lines are just multiple lines, eenly spaced
void ofxGCode::thick_line(float x1, float y1, float x2, float y2, float spacing, int layers){
    thick_line(ofVec2f(x1,y1), ofVec2f(x2,y2), spacing, layers);
}

void ofxGCode::thick_line(ofVec2f base_a, ofVec2f base_b, float spacing, int layers){
    //get the angle of the line
    float angle = atan2(base_a.y-base_b.y, base_a.x-base_b.x);
    float tan_angle = angle + PI/2;
    
    //draw it
    float dist_offset = spacing * (layers-1) * 0.5;
    
    
    for (int t=0; t<layers; t++){
        float dist = t * spacing - dist_offset;
        //cout<<"my dist "<<dist<<endl;
        ofVec2f a = ofVec2f(base_a);
        ofVec2f b = ofVec2f(base_b);
        a.x += cos(tan_angle) * dist;
        a.y += sin(tan_angle) * dist;
        b.x += cos(tan_angle) * dist;
        b.y += sin(tan_angle) * dist;
        line(a,b);
    }
}

vector<ofVec2f> ofxGCode::resample_lines(vector<ofVec2f> src_pnts, float sample_dist, bool close_shape, int steps_per_point){
    vector<ofVec2f> new_pnts;
    int end_index = close_shape ? src_pnts.size() : src_pnts.size()-1;
    
    float cur_dist = 0;
    ofVec2f prev_pos = ofVec2f(src_pnts[0]);
    new_pnts.push_back(src_pnts[0]);
    
    for (int i=0; i<end_index; i++){
        ofVec2f a = src_pnts[i];
        ofVec2f b = src_pnts[ (i+1)%src_pnts.size()];
        
        for (int k=0; k<=steps_per_point; k++){
            float prc = (float)k / (float)steps_per_point;
            ofVec2f pnt = (1.0-prc)*a + prc*b;
            cur_dist += ofDist(prev_pos.x, prev_pos.y, pnt.x, pnt.y);
            if (cur_dist >= sample_dist){
                new_pnts.push_back(pnt);
                cur_dist -= sample_dist;
            }
            prev_pos = ofVec2f(pnt);
        }
    }
    
    return new_pnts;
}

vector<GLine> ofxGCode::pnts_to_lines(vector<ofVec2f> pnts, bool close){
    vector<GLine> new_lines;
    for (int i=0; i<pnts.size()-1; i++){
        new_lines.push_back(GLine(pnts[i], pnts[i+1]));
    }
    
    if (close){
        new_lines.push_back(GLine(pnts[pnts.size()-1], pnts[0]));
    }
    
    return new_lines;
}

//Bezier Curves
void ofxGCode::bezier(ofVec2f p1, ofVec2f c1, ofVec2f c2, ofVec2f p2, int steps){
    vector<ofVec2f> pnts = get_bezier_pnts(p1, c1, c2, p2, steps);
    begin_shape();
    for (int i=0; i<pnts.size(); i++){
        vertex(pnts[i]);
    }
    end_shape(false);
}

vector<ofVec2f> ofxGCode::get_bezier_pnts(ofVec2f p1, ofVec2f c1, ofVec2f c2, ofVec2f p2, int steps){
    vector<ofVec2f> pnts;
    for (int i=0; i<=steps; i++){
        ofPoint pnt = ofBezierPoint(p1, c1, c2, p2, (float)i/(float)steps);
        pnts.push_back(pnt);
    }
    return pnts;
}

//be aware that this tool may no longer work
void ofxGCode::dot(float x, float y){
    line(x,y,x+0.01,y+0.01);
}


//https://openframeworks.cc/documentation/graphics/ofTrueTypeFont/#show_getStringAsPoints
void ofxGCode::text(string text, ofTrueTypeFont * font, float x, float y){
    bool vflip = true; // OF flips y coordinate in the default perspective,
    // should be false if using a camera for example
    bool filled = false; // or false for contours
    vector < ofPath > paths = font->getStringAsPoints(text, vflip, filled);
    
    ofPushMatrix();
    ofTranslate(x,y);
    
    for (int i = 0; i < paths.size(); i++){
        // for every character break it out to polylines
        vector <ofPolyline> polylines = paths[i].getOutline();
        
        // for every polyline, draw lines
        for (int j = 0; j < polylines.size(); j++){
            for (int k = 0; k < polylines[j].size(); k++){        
                int next_id = (k+1) % polylines[j].size();
                line(polylines[j][k].x,polylines[j][k].y, polylines[j][next_id].x,polylines[j][next_id].y);
            }
        }
    }
    
    ofPopMatrix();
}


//This function is by Andy, it attempts to recreate the functionality of modelX() and modelY() in Processing
//Currently it only works in 2D. 3D transformations will break it.
//it sometimes gets locked at 90 degree angles when the actual angle is 89 or 91. Not sure why
//it could definitely be more efficient by using quaternions properly
ofVec2f ofxGCode::getModelPoint(const ofVec3f & pnt){
    return getModelPoint(pnt.x, pnt.y);
}


ofVec2f ofxGCode::getModelPoint(float x, float y){
	ofVec4f pt = ofVec4f(x,y,0,1);
	ofVec4f pt2 = ofMatrix * pt + ofVec4f(ofGetViewportWidth()/2, ofGetViewportHeight()/2, 0, 0);
	return ofVec2f(pt2.x, pt2.y);
}


ofRectangle ofxGCode::calculateCanvasSize(){

	ofRectangle totalArea;
	bool firstLine = true;
	for (auto & l : lines){
		if(!l.skip_me){
			ofRectangle lineArea = ofRectangle(l.a.x, l.a.y, l.b.x - l.a.x, l.b.y - l.a.y);
			if(firstLine){
				firstLine = false;
				totalArea = lineArea;
			}else{
				totalArea = totalArea.getUnion(lineArea);
			}
		}
	}

	usedCanvas = totalArea;
	return totalArea;
}

//this is not perfect yet. Some of the resulting order is definitely not as efficient as it could be
void ofxGCode::sort(){
    
    //try to break the lines into groups of continuous lines
    vector<GCodeLineGroup> line_groups;
    GCodeLineGroup cur_group;
    while (lines.size() > 0){
        //if cur group is empty just grab the next line segment
        if (cur_group.lines.size() == 0){
            cur_group.add_to_front(lines[0]);
            lines.erase(lines.begin());
        }
        
        //go through and find segments that we can slap on the front or end
        bool added_any = false;
        for (int i=lines.size()-1; i>=0; i--){
            if (lines[i].a == cur_group.end_pos){
                cur_group.add_to_back(lines[i]);
                lines.erase(lines.begin()+i);
                added_any = true;
            }
            else if (lines[i].b == cur_group.start_pos){
                cur_group.add_to_front(lines[i]);
                lines.erase(lines.begin()+i);
                added_any = true;
            }
        }
        
        //if we added any, keep going, otherwise bail
        if (!added_any){
            line_groups.push_back(cur_group);
            cur_group.clear();
        }
    }
    //when we're done, add the last group if there's anything in it
    if (cur_group.lines.size() > 0){
        line_groups.push_back(cur_group);
    }
    
    
    //go through finding the closest end point
    ofVec2f cur_pnt = ofVec2f();
    while (line_groups.size() > 0){
        int close_id=0;
        float close_dist_sq = 9999999;
        bool need_to_flip = false;
        
        //check each unsorted line group
        for (int i=0; i<line_groups.size(); i++){
			const auto & thisGroup = line_groups[i];
            float dist_sq_a = ofDistSquared(thisGroup.start_pos.x, thisGroup.start_pos.y, cur_pnt.x, cur_pnt.y);
            
            //only get distance to B if it is OK to flip this line
            float dist_sq_b = 99999999;
            bool can_reverse = !thisGroup.do_not_reverse;
            if (can_reverse){
                dist_sq_b = ofDistSquared(thisGroup.end_pos.x, thisGroup.end_pos.y, cur_pnt.x, cur_pnt.y);
            }
            
            //are either of these poitns the closest so far?
            if (dist_sq_b < close_dist_sq){
                close_dist_sq = dist_sq_b;
                need_to_flip = true;
                close_id = i;
            }
            if (dist_sq_a < close_dist_sq){
                close_dist_sq = dist_sq_a;
                need_to_flip = false;
                close_id = i;
            }
        }
        
        GCodeLineGroup & group = line_groups[close_id];
        
        if (need_to_flip){
            for (int i=group.lines.size()-1; i>=0; i--){
                group.lines[i].swap_a_and_b();
                lines.push_back(group.lines[i]);
            }
            cur_pnt = group.start_pos;
        }else{
            for (int i=0; i<group.lines.size(); i++){
                lines.push_back(group.lines[i]);
            }
            cur_pnt = group.end_pos;
        }
        
        //remove it from unsorted
        //cout<<"remove "<<close_id<<" out of "<<line_groups.size()<<endl;
        //if (close_id <  line_groups.size()){
        line_groups.erase(line_groups.begin() + close_id);
        //}
    }
}

//sets all current lines so they cannot be trimmed or set as outwards only
void  ofxGCode::lock_lines(){
    for (int i=0; i<lines.size(); i++){
        lines[i].set_locked(true);
    }
}

void  ofxGCode::unlock_lines(){
    for (int i=0; i<lines.size(); i++){
        lines[i].set_locked(false);
    }
}

float ofxGCode::measureTransitDistance(){
    float distance = 0.0;
    
    for (int i=0; i<lines.size(); i++){
        distance += ofDist(lines[i].a.x, lines[i].a.y, lines[i].b.x, lines[i].b.y);
    }
    
    return distance;
}

//takes any vector of lines and returns a new vector where the spaces inside the polygon have been trimmed
vector<GLine> ofxGCode::trim_lines_inside(vector<GLine> lines, vector<ofVec2f> bounds){
    vector<GLine> output;
    
    //go through each line and try to trim it
    for (int i=0; i<lines.size(); i++){
        //trim it, adding any extra lines generated to the output
        lines[i].trim_inside(bounds, &output);
        //then if this line is still valid, add it as well
        if (!lines[i].skip_me){
            output.push_back(lines[i]);
        }
    }
    
    return output;
}

//trims the current list of lines, removing any points inside the given polygon
void ofxGCode::trim_inside(vector<ofVec2f> bounds){
    lines = trim_lines_inside(lines, bounds);
}

//helper function for trimming rectangles
vector<GLine> ofxGCode::trim_lines_inside(vector<GLine> lines, ofRectangle bounds){
    vector<ofVec2f> pnts;
    pnts.push_back(ofVec2f(bounds.x, bounds.y));
    pnts.push_back(ofVec2f(bounds.x+bounds.width, bounds.y));
    pnts.push_back(ofVec2f(bounds.x+bounds.width, bounds.y+bounds.height));
    pnts.push_back(ofVec2f(bounds.x, bounds.y+bounds.height));
    return trim_lines_inside(lines, pnts);
}
void ofxGCode::trim_inside(ofRectangle bounds){
    lines = trim_lines_inside(lines, bounds);
}

//takes any vector of lines and returns a new vector where any lines outside the shape have been removed
vector<GLine> ofxGCode::trim_lines_outside(vector<GLine> lines, vector<ofVec2f> bounds){
    vector<GLine> output;
    
    //go through each line and try to trim it
    for (int i=0; i<lines.size(); i++){
        //trim it, adding any extra lines generated to the output
        lines[i].trim_outside(bounds, &output);
        //then if this line is still valid, add it as well
        if (!lines[i].skip_me){
            output.push_back(lines[i]);
        }
    }
    
    return output;
}

//trims the current list of lines, removing any points outside the given polygon
void ofxGCode::trim_outside(vector<ofVec2f> bounds){
    lines = trim_lines_outside(lines, bounds);
}

//helper function for trimming rectangles
vector<GLine> ofxGCode::trim_lines_outside(vector<GLine> lines, ofRectangle bounds){
    vector<ofVec2f> pnts;
    pnts.push_back(ofVec2f(bounds.x, bounds.y));
    pnts.push_back(ofVec2f(bounds.x+bounds.width, bounds.y));
    pnts.push_back(ofVec2f(bounds.x+bounds.width, bounds.y+bounds.height));
    pnts.push_back(ofVec2f(bounds.x, bounds.y+bounds.height));
    return trim_lines_outside(lines, pnts);
}
void ofxGCode::trim_outside(ofRectangle bounds){
    lines = trim_lines_outside(lines, bounds);
}

//takes a list of lines and rmeoves any lines that intersect a satic line
vector<GLine> ofxGCode::trim_intersecting_lines(vector<GLine> lines_to_trim, vector<GLine> static_lines){
    vector<GLine> val;
    for (int i=0; i<lines_to_trim.size(); i++){
        bool can_add = true;
        for (int k=0; k<static_lines.size(); k++){
            if (lines_to_trim[i].intersects(static_lines[k])){
                can_add = false;
                break;
            }
        }
        if (can_add){
            val.push_back(lines_to_trim[i]);
        }
    }
    return  val;
}

void ofxGCode::demo_trim(float x1, float y1, float x2, float y2, bool do_translate){
    //first trim in the box
    ofRectangle box;
    box.x = x1;
    box.y = y1;
    box.width = x2-x1;
    box.height = y2-y1;
    
    trim_outside(box);
    
    if (do_translate){
        translate(-x1, -y1);
    }
}

//--------------------------------------------------------------
//any lines outside of this bounds will be forced to draw from the center out.
void ofxGCode::set_outwards_only_bounds(ofRectangle safe_area){
    
    for (int i=0; i<lines.size(); i++){
        if (!lines[i].is_locked){
            bool a_inside = safe_area.inside(lines[i].a);
            bool b_inside = safe_area.inside(lines[i].b);
            //if both sides are in the safe area, do nothing. We can flip this line if we need to
            if (a_inside && b_inside){
                lines[i].do_not_reverse = false;
            }
            
            //if only A is inside, keep the order but make sure it doesn't get flipped
            else if (a_inside && !b_inside){
                lines[i].do_not_reverse = true;
            }
            
            //if only B is inside, flip it and make sure it does not get flipped again
            else if (b_inside && !a_inside){
                lines[i].swap_a_and_b();
                lines[i].do_not_reverse = true;
            }
            
            //if neither is inside, see if the center point of the line is inside and attempt to split it
            else{
                ofVec2f mid = lines[i].a*0.5 + lines[i].b*0.5;
                bool mid_inside = safe_area.inside(mid);
                //if midpoint is inside, split it!
                if (mid_inside){
                    //make a new line
                    GLine new_line = GLine(mid, lines[i].b);
                    lines.push_back(new_line);
                    //trim this line
                    lines[i].b = mid;
                    //push i back so we re-evaluate this line
                    i--;
                }
                //otherwise, give up. select the point closest to the center and have that be A
                else{
                    ofVec2f center;
                    center.x = (safe_area.x + safe_area.x+safe_area.width)/2;
                    center.y = (safe_area.y + safe_area.y+safe_area.height)/2;
                    if (center.squareDistance(lines[i].a) > center.squareDistance(lines[i].b)){
                        lines[i].swap_a_and_b();
                    }
                    lines[i].do_not_reverse = true;
                }
            }
            
            
        }
    }
    
}

//--------------------------------------------------------------
void ofxGCode::translate(float x, float y){
    for (int i=0; i<lines.size(); i++){
        if (!lines[i].is_locked){
            lines[i].a.x += x;
            lines[i].a.y += y;
            lines[i].b.x += x;
            lines[i].b.y += y;
        }
    }
}

//--------------------------------------------------------------
//0-top left, 1-top right, 2-bottom right, 3-bottom left
ofVec2f ofxGCode::perspective_warp(ofVec2f orig_pnt, ofRectangle src_bounds, ofVec2f new_bounds[4], float x_curve, float y_curve){
    
    //get the percentage of the x and y in the original box
    float x_prc = (orig_pnt.x-src_bounds.x) / src_bounds.width;
    float y_prc = (orig_pnt.y-src_bounds.y) / src_bounds.height;
    
    x_prc = powf(x_prc, x_curve);
    y_prc = powf(y_prc, y_curve);
    
    //now move along the top and bottom of the new shape to the same x_prc
    ofVec2f top_pnt = (1.0-x_prc)*new_bounds[0] + x_prc*new_bounds[1];
    ofVec2f bot_pnt = (1.0-x_prc)*new_bounds[3] + x_prc*new_bounds[2];
    
    //now lerp between those based on the y
    ofVec2f new_pos = (1.0-y_prc)*top_pnt + y_prc*bot_pnt;
    
    
    return new_pos;
}

//--------------------------------------------------------------
vector<vector<ofVec2f>> ofxGCode::load_outlines(string file_path){
    vector<vector<ofVec2f>> outlines;
    
    ofFile file(file_path);
    
    if(!file.exists()){
        cout<<"The outline file " << file_path << " is missing"<<endl;
        return outlines;
    }
    ofBuffer buffer(file);
    
    //Read file line by line
    vector<ofVec2f> cur_outline;
    for (ofBuffer::Line it = buffer.getLines().begin(), end = buffer.getLines().end(); it != end; ++it) {
        string line = *it;
        //Split line into strings
        vector<string> words = ofSplitString(line, ",");

        if (words.size()>0){

            //using # to mark the start of a new shape
            if (words[0]=="#"){
                if (cur_outline.size() > 1){
                    outlines.push_back(cur_outline);
                    cur_outline.clear();
                }
            }
            //there should be two words
            else if (words.size()==2){
                ofVec2f pnt;
                pnt.x = ofToFloat(words[0]);
                pnt.y = ofToFloat(words[1]);
                cur_outline.push_back(pnt);
            }
        }

    }
    
    //add the last shape if there's anything there
    cout<<cur_outline.size()<<endl;
    if (cur_outline.size() > 1){
        outlines.push_back(cur_outline);
    }
    
    return outlines;
}

//--------------------------------------------------------------
vector<GLine> ofxGCode::load_lines(string file_path){
    vector<GLine> new_lines;
    
    ofFile file(file_path);
    
    if(!file.exists()){
        cout<<"The file " << file_path << " is missing"<<endl;
        return new_lines;
    }
    ofBuffer buffer(file);
    
    //Read file line by line
    for (ofBuffer::Line it = buffer.getLines().begin(), end = buffer.getLines().end(); it != end; ++it) {
        string line = *it;
        //Split line into strings
        vector<string> words = ofSplitString(line, ",");
        
        if (words.size()==4){
            GLine line;
            line.a.x =  ofToFloat(words[0]);
            line.a.y =  ofToFloat(words[1]);
            line.b.x =  ofToFloat(words[2]);
            line.b.y =  ofToFloat(words[3]);
            new_lines.push_back(line);
        }
        
    }
    return new_lines;
    
}

//--------------------------------------------------------------
void ofxGCode::save_lines(string file_path){
    ofFile myTextFile;
    myTextFile.open(file_path,ofFile::WriteOnly);
    for (int i=0; i<lines.size(); i++){
        myTextFile<<lines[i].a.x<<","<<lines[i].a.y<<","<<lines[i].b.x<<","<<lines[i].b.y<<endl;
    }
}

//--------------------------------------------------------------
//code is a modified version of code by Randolph Franklin
//from http://paulbourke.net/geometry/insidepoly/
bool ofxGCode::checkInPolygon(vector<ofVec2f> p, float x, float y)
{
    int i, j, c = 0;
    for (i = 0, j = p.size()-1; i < p.size(); j = i++) {
        if ((((p[i].y <= y) && (y < p[j].y)) ||
             ((p[j].y <= y) && (y < p[i].y))) &&
            (x < (p[j].x - p[i].x) * (y - p[i].y) / (p[j].y - p[i].y) + p[i].x))
            c = !c;
    }
    return c;
}





