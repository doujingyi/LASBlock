#include"LASBlock.h"

unsigned GetFormatRecordLength(uint8_t pointFormat)
{
	switch (pointFormat)
	{
	case 0:
		return 20;              //0 - base
	case 1:
		return 20 + 8;          //1 - base + GPS
	case 2:
		return 20 + 6;          //2 - base + RGB
	case 3:
		return 20 + 8 + 6;      //3 - base + GPS + RGB
	case 4:
		return 20 + 8 + 29;     //4 - base + GPS + FWF
	case 5:
		return 20 + 8 + 6 + 29; //5 - base + GPS + FWF + RGB
	case 6:
		return 30;              //6  - base (GPS included)
	case 7:
		return 30 + 6;          //7  - base + RGB
	case 8:
		return 30 + 6 + 2;      //8  - base + RGB + NIR (not used)
	case 9:
		return 30 + 29;         //9  - base + FWF
	case 10:
		return 30 + 6 + 2 + 29; //10 - base + RGB + NIR + FWF
	default:
		assert(false);
		return 0;
	}
}

void LASBlock::setParam(params param)
{
	BlockParam = param;
	std::vector<std::string> lasfile_dir;
	std::ifstream fp(param.input_dir.c_str());
	while (!fp.eof()) {
		std::string filename;
		std::getline(fp,filename);
		if (filename.size() > 0)
		{
			size_t pos = 0;
			pos = filename.find_last_of("\r");
			//if we read a windows file(txt \r\n)
			if(pos>=0 && pos<filename.size())
			{
				filename = filename.substr(0,pos);
			}
			lasfile_dir.emplace_back(filename);
		}
	}
	fp.close();

	if (lasfile_dir.size() == 0) {
		std::cerr << "ERROR!,The las file is empty!" << std::endl;
		exit(-1);
	}

	int number_las = lasfile_dir.size();
	LasOriginBoundary_info.resize(number_las);

	for (int i = 0; i < number_las; i++)
	{
		std::string lasfile_name = lasfile_dir[i];
		LasOriginBoundary_info[i].first = lasfile_name;

		LASreadOpener lasreadopener;
		lasreadopener.set_file_name(lasfile_name.c_str());

		if (!lasreadopener.active()) {
			std::cout << "ERROR: no input specified" << std::endl;
			exit(-1);
		}

		LASreader* lasreader = lasreadopener.open();
		if (lasreader == 0) {
			std::cout << "ERROR: could not open lasreader" << std::endl;
			exit(-1);
		}

		LASinfo  las_info;
		las_info.min_x = lasreader->get_min_x();
		las_info.min_y = lasreader->get_min_y();
		las_info.min_z = lasreader->get_min_z();
		las_info.max_x = lasreader->get_max_x();
		las_info.max_y = lasreader->get_max_y();
		las_info.max_z = lasreader->get_max_z();

		las_info.x_scale_factor = lasreader->header.x_scale_factor;
		las_info.y_scale_factor = lasreader->header.y_scale_factor;
		las_info.z_scale_factor = lasreader->header.z_scale_factor;
		las_info.x_offset = lasreader->header.x_offset;
		las_info.y_offset = lasreader->header.y_offset;
		las_info.z_offset = lasreader->header.z_offset;

		las_info.version_major = lasreader->header.version_major;
		las_info.version_minor = lasreader->header.version_minor;
		las_info.point_data_format = lasreader->header.point_data_format;
		if (las_info.point_data_format < 2) {
			las_info.point_data_format += 2;
		}
		else if (las_info.point_data_format == 4) {
			las_info.point_data_format = 5;
		}
		las_info.point_data_record_length = GetFormatRecordLength(las_info.point_data_format);
		LasOriginBoundary_info[i].second = las_info;

		delete lasreader;
	}
}

bool LASBlock::isIntersect(LASinfo rectA, LASinfo rectB)
{
	////The point where rectA and rectB intersect with the greater value in the X direction and the greater value in the Y direction in topleft
	//F64 topleft_maxx = rectA.min_x > rectB.min_x ? rectA.min_x : rectB.min_x;
	//F64 topleft_maxy = rectA.min_y > rectB.min_y ? rectA.min_y : rectB.min_y;
	//F64 bottomright_minx = rectA.max_x < rectB.max_x ? rectA.max_x : rectB.max_x;
	//F64 bottomright_miny = rectA.max_y < rectB.max_y ? rectA.max_y : rectB.max_y;

	//if (topleft_maxx < bottomright_minx&&topleft_maxy < bottomright_miny)
	//	return true;
	//else
	//	return false;

	//The Envelope method
	LASinfo EnvelopeRect;
	EnvelopeRect.min_x = rectA.min_x < rectB.min_x ? rectA.min_x : rectB.min_x;
	EnvelopeRect.min_y = rectA.min_y < rectB.min_y ? rectA.min_y : rectB.min_y;
	EnvelopeRect.max_x = rectA.max_x > rectB.max_x ? rectA.max_x : rectB.max_x;
	EnvelopeRect.max_y = rectA.max_y > rectB.max_y ? rectA.max_y : rectB.max_y;

	F64 rectA_width = rectA.max_x - rectA.min_x;
	F64 rectA_height = rectA.max_y - rectA.min_y;
	F64 rectB_width = rectB.max_x - rectB.min_x;
	F64 rectB_height = rectB.max_y - rectB.min_y;
	F64 EnvelopeRect_width = EnvelopeRect.max_x - EnvelopeRect.min_x;
	F64 EnvelopeRect_height = EnvelopeRect.max_y - EnvelopeRect.min_y;

	if (EnvelopeRect_width<= rectA_width+ rectB_width&& EnvelopeRect_height<= rectA_height+ rectB_height)
		return true;
	else
		return false;
}

void LASBlock::divide()
{
	std::string output_poxtfix = BlockParam.poxtfix == 0 ? ".las" : ".laz";

	/* caculate the maximum range coordnates*/
	std::vector<F64> sort_x, sort_y;
	for (auto it = LasOriginBoundary_info.begin(); it < LasOriginBoundary_info.end(); it++)
	{
		sort_x.emplace_back((*it).second.min_x);
		sort_x.emplace_back((*it).second.max_x);
		sort_y.emplace_back((*it).second.min_y);
		sort_y.emplace_back((*it).second.max_y);
	}
	std::sort(sort_x.begin(), sort_x.end());
	std::sort(sort_y.begin(), sort_y.end());

	I64 BoundaryMinX = std::round(sort_x[0]);
	I64 BoundaryMaxX = std::round(sort_x.back());
	I64 BoundaryMinY = std::round(sort_y[0]);
	I64 BoundaryMaxY = std::round(sort_y.back());

	/*Divide subtile*/
	I64 integer_minx = BoundaryMinX - BoundaryMinX % 1000;
	I64 integer_miny = BoundaryMinY - BoundaryMinY % 1000;
	I64 x_block_num = std::ceil((BoundaryMaxX - integer_minx) / BlockParam.tile_size);
	I64 y_block_num = std::ceil((BoundaryMaxY - integer_miny) / BlockParam.tile_size);

	std::vector<LASinfo> origin_subtile(x_block_num*y_block_num);
	for (int i = 0; i < x_block_num; i++)
		for (int j = 0; j < y_block_num; j++)
		{
			LASinfo current_block;
			current_block.min_x = integer_minx + BlockParam.tile_size * i;
			current_block.min_y = integer_miny + BlockParam.tile_size * j;
			current_block.max_x = current_block.min_x + BlockParam.tile_size;
			current_block.max_y = current_block.min_y + BlockParam.tile_size;
			origin_subtile[i + j * x_block_num] = current_block;
		}

	//label nonpoint block and filter them and if keep_xy==ture, we must filter subblocks which are not meet requirments 
	std::vector<bool> retain_list(origin_subtile.size(), false);
	for (int i = 0; i < origin_subtile.size(); i++)
	{
		LASinfo current_sub_block = origin_subtile[i];
		for (int j = 0; j < LasOriginBoundary_info.size(); j++)
		{
			LASinfo current_orign_block = LasOriginBoundary_info[j].second;
			bool flag = isIntersect(current_sub_block, current_orign_block);
			if (BlockParam.keep_xy && flag)
			{
				LASinfo range_block;
				range_block.min_x = BlockParam.range_min_x;
				range_block.min_y = BlockParam.range_min_y;
				range_block.max_x = BlockParam.range_max_x;
				range_block.max_y = BlockParam.range_max_y;
				flag = isIntersect(current_sub_block, range_block);
			}
			if (flag == true) {
				retain_list[i] = true;
				break;
			}
		}
	}
	//fiter nonpoint tile and record information which each tile contain las files
	int tile_num = 0;
	for (int i = 0; i < retain_list.size(); i++)
	{
		std::pair<std::string, LASinfo> tmp;
		if (retain_list[i] == true) {
			tmp.first = BlockParam.output_dir+"/"+BlockParam.output_prefix + std::to_string((int)origin_subtile[i].min_x) + "_" + std::to_string((int)origin_subtile[i].min_y) + output_poxtfix;
			tmp.second = origin_subtile[i];
			LasRetainBoundary_info.emplace_back(tmp);
			tile_num++;
		}
	}

	std::vector<std::vector<LASinfo>> subtile_contain_lasinfo;
	subtile_contain_laspath.resize(tile_num);
	subtile_contain_lasinfo.resize(tile_num);//Temporary variables
	for (int i = 0; i < tile_num; i++)
	{
		LASinfo current_sub_block = LasRetainBoundary_info[i].second;
		current_sub_block.min_x -= BlockParam.buffer_width;
		current_sub_block.min_y -= BlockParam.buffer_width;
		current_sub_block.max_x += BlockParam.buffer_width;
		current_sub_block.max_y += BlockParam.buffer_width;

		for (int j = 0; j < LasOriginBoundary_info.size(); j++)
		{
			LASinfo current_orign_block = LasOriginBoundary_info[j].second;
			bool flag = isIntersect(current_sub_block, current_orign_block);
			if (flag == true) {
				subtile_contain_laspath[i].push_back(LasOriginBoundary_info[j].first);
				subtile_contain_lasinfo[i].push_back(LasOriginBoundary_info[j].second);
			}
		}
		std::vector<F64> x_scale_factor_l, y_scale_factor_l, z_scale_factor_l, x_offset_l, y_offset_l, z_offset_l;
		std::vector<U8>point_data_format_l;

		LasRetainBoundary_info[i].second.version_major = subtile_contain_lasinfo[i][0].version_major;
		LasRetainBoundary_info[i].second.version_minor = subtile_contain_lasinfo[i][0].version_minor;

		for (int j = 0; j < subtile_contain_lasinfo[i].size(); j++) {
			x_scale_factor_l.emplace_back(subtile_contain_lasinfo[i][j].x_scale_factor);
			y_scale_factor_l.emplace_back(subtile_contain_lasinfo[i][j].y_scale_factor);
			z_scale_factor_l.emplace_back(subtile_contain_lasinfo[i][j].z_scale_factor);
			x_offset_l.emplace_back(subtile_contain_lasinfo[i][j].x_offset);
			y_offset_l.emplace_back(subtile_contain_lasinfo[i][j].y_offset);
			z_offset_l.emplace_back(subtile_contain_lasinfo[i][j].z_offset);
			point_data_format_l.emplace_back(subtile_contain_lasinfo[i][j].point_data_format);
		}
		std::sort(x_scale_factor_l.begin(), x_scale_factor_l.end());
		std::sort(y_scale_factor_l.begin(), y_scale_factor_l.end());
		std::sort(z_scale_factor_l.begin(), z_scale_factor_l.end());
		std::sort(x_offset_l.begin(), x_offset_l.end());
		std::sort(y_offset_l.begin(), y_offset_l.end());
		std::sort(z_offset_l.begin(), z_offset_l.end());
		std::sort(point_data_format_l.begin(), point_data_format_l.end());

		LasRetainBoundary_info[i].second.x_scale_factor = x_scale_factor_l.back();
		LasRetainBoundary_info[i].second.y_scale_factor = y_scale_factor_l.back();
		LasRetainBoundary_info[i].second.z_scale_factor = z_scale_factor_l.back();
		LasRetainBoundary_info[i].second.x_offset = x_offset_l.back();
		LasRetainBoundary_info[i].second.y_offset = y_offset_l.back();
		LasRetainBoundary_info[i].second.z_offset = z_offset_l.back();
		LasRetainBoundary_info[i].second.point_data_format = point_data_format_l.back();
		LasRetainBoundary_info[i].second.point_data_record_length = GetFormatRecordLength(point_data_format_l.back());
	}

   	std::cout << "Finished divide tiles!"<<std::endl;
} 

bool LASBlock::singleLasDivideTask(std::pair<std::string, LASinfo> subtile_info, std::vector<std::string> tile_contain_laspath)
{
	std::string lastile_path = subtile_info.first;

	LASwriteOpener laswriteopener;
	LASheader            las_header_write;
	LASwriter*           laswriter;
	laswriteopener.set_file_name(lastile_path.c_str());

	if (!laswriteopener.active()) {
		std::cerr << "error: could not write las file: " << lastile_path << std::endl;
		exit(-1);
	}

	if (strcmp(&lastile_path.back(), "z") == 0) {
		laswriteopener.set_format(LAS_TOOLS_FORMAT_LAZ);
	}
	else {
		laswriteopener.set_format(LAS_TOOLS_FORMAT_LAS);
	}

	las_header_write.x_offset = subtile_info.second.x_offset;
	las_header_write.y_offset = subtile_info.second.y_offset;
	las_header_write.z_offset = subtile_info.second.z_offset;
	las_header_write.x_scale_factor = subtile_info.second.x_scale_factor;
	las_header_write.y_scale_factor = subtile_info.second.y_scale_factor;
	las_header_write.z_scale_factor = subtile_info.second.z_scale_factor;

	las_header_write.version_major = subtile_info.second.version_major; // 1
	las_header_write.version_minor = subtile_info.second.version_minor; // 2

	las_header_write.point_data_format = subtile_info.second.point_data_format;
	las_header_write.point_data_record_length = subtile_info.second.point_data_record_length;

	 laswriter = laswriteopener.open(&las_header_write);
	if (!laswriter) {
		return false;
	}

	LASpoint laspoint_w;
	if (!laspoint_w.init(&las_header_write, las_header_write.point_data_format, las_header_write.point_data_record_length, 0)) {
		return EXIT_FAILURE;
	}
	// write points
	double minX = DBL_MAX, minY = DBL_MAX, minZ = DBL_MAX;
	double maxX = -DBL_MAX, maxY = -DBL_MAX, maxZ = -DBL_MAX;

	for (int i = 0; i < tile_contain_laspath.size(); i++)
	{
		LASreadOpener lasreadopener;
		LASheader     las_header_read;
		LASreader     *lasreader;

		// Set maximum number of points to read
		double x, y, z;
		U8 intensity=0;
		U16 classification=0;
		int max_num = INT_MAX;
		int num_read = 0;
		
		// Open the LAS file
		lasreadopener.set_file_name(tile_contain_laspath[i].c_str());

		if (!lasreadopener.active()) {
			std::cout << "ERROR: no input specified" << std::endl;
			return false;
		}

		lasreader = lasreadopener.open();
		if (!lasreader) {
			std::cerr << "ERROR: could not open LAS file: " << tile_contain_laspath[i] << std::endl;
			return false;
		}

		las_header_read = lasreader->header;
		las_header_read.user_data_after_header = nullptr;
		U8 point_type = las_header_read.point_data_format;
		U16 point_size = las_header_read.point_data_record_length;

		LASpoint point_r;
		point_r.init(&las_header_read, point_type, point_size, &las_header_read);
		
		while (lasreader->read_point() && num_read < max_num) 
		{
			num_read++;
			point_r = lasreader->point;

			x = point_r.get_x();
			y = point_r.get_y();
			z = lasreader->point.get_z();
			if (BlockParam.keep_xy)
			{
				if(!(x>= BlockParam.range_min_x&&x<=BlockParam.range_max_x&&y>=BlockParam.range_min_y&&y<=BlockParam.range_max_y))
					continue;
			}
			else if (BlockParam.keep_z)
			{
				if(z<BlockParam.range_min_z||z>BlockParam.range_max_z)
					continue;
			}

			intensity = point_r.get_intensity();
			classification = point_r.get_classification();

			if ((x >= subtile_info.second.min_x - BlockParam.buffer_width) && (x <= subtile_info.second.max_x + BlockParam.buffer_width)
				&& (y >= subtile_info.second.min_y - BlockParam.buffer_width) && (y <= subtile_info.second.max_y + BlockParam.buffer_width))
			{
				if (point_r.have_rgb) {
					laspoint_w.set_R(point_r.rgb[0]);
					laspoint_w.set_G(point_r.rgb[1]);
					laspoint_w.set_B(point_r.rgb[2]);
					laspoint_w.set_I(point_r.rgb[3]);
				}
				else if (point_r.have_gps_time)
					laspoint_w.set_gps_time(point_r.gps_time);
				else if (point_r.have_nir)
					laspoint_w.set_NIR(point_r.get_NIR());

				laspoint_w.set_X((x - las_header_write.x_offset) / las_header_write.x_scale_factor);
				laspoint_w.set_Y((y - las_header_write.y_offset) / las_header_write.y_scale_factor);
				laspoint_w.set_Z((z - las_header_write.z_offset) / las_header_write.z_scale_factor);
				laspoint_w.set_return_number(point_r.return_number);
				laspoint_w.set_number_of_returns(point_r.number_of_returns);
				laspoint_w.set_scan_direction_flag(point_r.get_scan_direction_flag());
				laspoint_w.set_edge_of_flight_line(point_r.get_edge_of_flight_line());
				laspoint_w.set_classification(classification);
				laspoint_w.set_synthetic_flag(point_r.get_synthetic_flag());
				laspoint_w.set_keypoint_flag(point_r.keypoint_flag);
				laspoint_w.set_withheld_flag(point_r.get_withheld_flag());
				laspoint_w.set_scan_angle_rank(point_r.get_scan_angle_rank());
				laspoint_w.set_user_data(point_r.get_user_data());
				laspoint_w.set_point_source_ID(point_r.point_source_ID);
				laspoint_w.set_scan_angle(point_r.get_scan_angle());		
				laspoint_w.set_intensity(intensity);

				laswriter->write_point(&laspoint_w);
				laswriter->update_inventory(&laspoint_w);
				laswriter->npoints++;
				// range
				if (x < minX) minX = x;
				if (x > maxX) maxX = x;
				if (y < minY) minY = y;
				if (y > maxY) maxY = y;
				if (z < minZ) minZ = z;
				if (z > maxZ) maxZ = z;
			}
		}
	
		// Close the LAS file
		lasreader->close();
		delete lasreader;
		lasreader = nullptr;
	}
	// update the boundary
	las_header_write.set_bounding_box(minX, minY, minZ, maxX, maxY, maxZ, false, false);

	// update the header
	laswriter->update_header(&las_header_write, true);

	laswriter->close();
	delete laswriter;
	laswriter = nullptr;

	return true;
}

void LASBlock::run()
{
#ifdef _WIN32
	int maxThreadNum = std::thread::hardware_concurrency();
#else defined linux
	int maxThreadNum = boost::thread::hardware_concurrency();
#endif
	int thread_num = BlockParam.ThreadNum;
	int tile_task = LasRetainBoundary_info.size();

	if (thread_num<1 || thread_num>maxThreadNum) {
		std::cerr << "ERROR! The number of thread must meet the requirements\n";
		exit(1);
	}
	if (thread_num ==1)
	{
		for (int i = 0; i < tile_task; i++)
		{
			singleLasDivideTask(LasRetainBoundary_info[i], subtile_contain_laspath[i]);
			std::cout << "Finished " << i+1 << " task" << std::endl;
		}
	}
	else
	{
		std::cout << "Total task is:" << tile_task << std::endl;
		for (int i=0;i< tile_task;i+= thread_num)
		{
#ifdef _WIN32
			std::vector<std::thread> threads;

			int remain_thread_num = tile_task - i  >= thread_num ? thread_num : tile_task - i ;
			std::cout << "The left task is:" << tile_task - i << std::endl;

			for (int j=0;j< remain_thread_num;j++)
				threads.push_back(std::thread(&LASBlock::singleLasDivideTask, this,LasRetainBoundary_info[i+j], subtile_contain_laspath[i+j]));

			for (int j = 0; j < remain_thread_num; j++)
				threads[j].join();
#else defined linux
			std::vector<boost::thread> threads;

			int remain_thread_num = tile_task - i  >= thread_num ? thread_num : tile_task - i ;
			std::cout << "The left task is:" << tile_task - i << std::endl;

			for (int j=0;j< remain_thread_num;j++)
				threads.push_back(boost::thread(&LASBlock::singleLasDivideTask, this,LasRetainBoundary_info[i+j], subtile_contain_laspath[i+j]));

			for (int j = 0; j < remain_thread_num; j++)
				threads[j].join();
#endif
			
		}
	}
	
}
