#include <iostream>
#include <fstream>

#include "algorithm"
#include "spdlog/fmt/ostr.h"
#include "spdlog/sinks/basic_file_sink.h"

struct SAM_format{
	std::string qname;
	std::string rname;
	std::string cigar;
	std::string rnext;
	std::string seq;
	std::string qual;
	int flag;
	int pos;
	int mapq;
	int pnext;
	int tlen;
/*	template<typename OStream>
    friend OStream &operator<<(OStream &os, const SAM_format &s){	
    	os << '*' << '\t' // QNAME
			<< '*' << '\t' // FLAGS
			<< s.rname << '\t' // RNAME
			<< s.pos + 1 << '\t' // pos (1-based)
			<< 255 << '\t' // MAPQ
			<< '*' << '\t' // CIGAR
			<< '*' << '\t' // MATE NAME
			<< 0 << '\t' // MATE pos
			<< s.tlen << '\t' // TLEN
			<< '*' << '\t' // SEQ
			<< "*\t" << '\n';// QSTR
    }*/

};
struct ref_idx_pos
{
	std::string rname;
	int pos_1;
	int pos_2;
	bool operator < (const ref_idx_pos &a)const 
	{  
	 return rname.compare(a.rname) < 0;
	}
};

bool sort_comp(const ref_idx_pos &a,const ref_idx_pos &b)
{
      return a < b;
}

class SAMwriter
{
private:
//	std::shared_ptr<spdlog::logger> samlog;
	mapped_res mres_1,mres_2;
	ref_start_name rsn;
	std::vector<ref_idx_pos> rip_vec_1,rip_vec_2;
	std::vector<std::string> read_name;
	std::string dim_str;
public:
	SAMwriter()
	{
		dim_str = std::to_string(DIM);
	//	samlog = spdlog::basic_logger_mt("basic_logger",SAM_FILE_LOC);
	//	samlog->set_pattern("..");
	}

	void read_mapped_info()
	{
		{
			std::ifstream read_name_file(PAIR_1_NAME_FILE);
			cereal::BinaryInputArchive ar_read_name(read_name_file);
			ar_read_name(read_name);
		}

		{
			std::ifstream ref_pos_file(MERGE_REF_POS_FILE);
			cereal::BinaryInputArchive ar_ref_pos(ref_pos_file);
			ar_ref_pos(rsn.ref_start,rsn.rname);
		}

		{
			std::ifstream locfile(PAIR_1_LOC_FILE);
			cereal::BinaryInputArchive ar_loc(locfile);
			ar_loc(mres_1.mapped_ref_loc);
		}

		{
			std::ifstream disfile(PAIR_1_DIS_FILE);
			cereal::BinaryInputArchive ar_dis(disfile);
			ar_dis(mres_1.min_dis);
		}

		{
			std::ifstream locfile(PAIR_2_LOC_FILE);
			cereal::BinaryInputArchive ar_loc(locfile);
			ar_loc(mres_2.mapped_ref_loc);
		}

		{
			std::ifstream disfile(PAIR_2_DIS_FILE);
			cereal::BinaryInputArchive ar_dis(disfile);
			ar_dis(mres_2.min_dis);
		}

	/*	std::ofstream tmp("ref_loc_name.txt");
		for (int i = 0; i < rsn.ref_start.size(); ++i){
			tmp << rsn.ref_start[i] << "\t" << rsn.rname[i] << std::endl;
		}
		tmp.close();*/
	}

// TODO
	void gen_SAM_file()
	{
		//mapped_res &mres_1){
		std::ifstream read_seq_file_1(INPUT_READ_FILE_NAME_1);
		std::ifstream read_seq_file_2(INPUT_READ_FILE_NAME_2);

		read_mapped_info();
		SAM_format sf_1,sf_2;
		std::ofstream samfile(SAM_FILE_LOC);

		for (int read_idx = 0; read_idx < mres_1.mapped_ref_loc.size(); ++read_idx){
		//for (int read_idx = 0; read_idx < 2; ++read_idx){
			//for (int loc = 0; loc < mres_1.mapped_ref_loc[read_idx].size(); ++loc){
			int loc = 0;
			auto size = max(mres_1.mapped_ref_loc[read_idx].size(),mres_2.mapped_ref_loc[read_idx].size());
			ref_idx_pos rip_1;
			ref_idx_pos rip_2;
			rip_vec_1.clear();
			rip_vec_2.clear();
			while(loc < size)
			{
			//	samlog->info(SAM_format{sam.qname,sam.rname,sam.cigar,sam.rnext,sam.seq,sam.qual,sam.flag,sam.pos,sam.mapq,sam.pnext,sam.tlen});

				// find the exact mapping position according to the size of transcriptomes
				if (loc < mres_1.mapped_ref_loc[read_idx].size() && mres_1.min_dis[read_idx] <= TOLERANCE)
				{
					
					for (int i = 0; i < rsn.ref_start.size(); ++i)
					{
						if (rsn.ref_start[i] <= mres_1.mapped_ref_loc[read_idx][loc] && rsn.ref_start[i + 1] > mres_1.mapped_ref_loc[read_idx][loc])
						{
							rip_1.rname = rsn.rname[i];
							rip_1.pos_1 = std::move(mres_1.mapped_ref_loc[read_idx][loc] - rsn.ref_start[i]);
						//	std::cout << ridx_1 << std::endl;
							break;
						}
					}
					rip_vec_1.push_back(rip_1);
				}

				if (loc < mres_2.mapped_ref_loc[read_idx].size() && mres_2.min_dis[read_idx] <= TOLERANCE)
				{
					
					for (int i = 0; i < rsn.ref_start.size(); ++i)
					{
						if (rsn.ref_start[i] <= mres_2.mapped_ref_loc[read_idx][loc] && rsn.ref_start[i + 1] > mres_2.mapped_ref_loc[read_idx][loc])
						{
							rip_2.rname = rsn.rname[i];
							rip_2.pos_2 = std::move(mres_2.mapped_ref_loc[read_idx][loc] - rsn.ref_start[i]);
						//	std::cout << ridx_2 << std::endl << std::endl;
							break;
						}
					}
					rip_vec_2.push_back(rip_2);
				}
				loc++;	
			}
			std::vector<ref_idx_pos> rip_vec;
		//	sort(rip_vec_1.begin(),rip_vec_1.end(),less<ref_idx_pos>());
		//	sort(rip_vec_2.begin(),rip_vec_2.end(),less<ref_idx_pos>());

		//	set_intersection(rip_vec_1.begin(),rip_vec_1.end(),rip_vec_2.begin(),rip_vec_2.end(),back_inserter(rip_vec),sort_comp);	
			for (int i = 0; i < rip_vec_1.size(); ++i)
			{
				for (int j = 0; j < rip_vec_2.size(); ++j)
				{
					if (rip_vec_1[i].rname.compare(rip_vec_2[j].rname) == 0)
					{
						rip_vec_1[i].pos_2 = std::move(rip_vec_2[j].pos_2);
						rip_vec.push_back(std::move(rip_vec_1[i]));
					}
				}
			}
			getline(read_seq_file_1,sf_1.seq);
			getline(read_seq_file_2,sf_2.seq);
			for (int i = 0; i < rip_vec.size(); ++i)
			{	
				sf_1.rname = std::move(rip_vec[i].rname);
				sf_1.pos = std::move(rip_vec[i].pos_1 + 1);
				sf_2.pos = std::move(rip_vec[i].pos_2 + 1);
				sf_1.tlen = DIM;
				sf_1.cigar = dim_str + "M";
				sf_2.cigar = dim_str + "M";
			/*	if (mres_1.min_dis[read_idx] == 0){
					sf_1.cigar = dim_str + "M";
				}else{
					sf_1.cigar = "X";
				}

				if (mres_2.min_dis[read_idx] == 0){
					sf_2.cigar = dim_str + "M";
				}else{
					sf_2.cigar = "X";
				}*/
				
				sf_1.qname = read_name[read_idx];	
				sf_1.tlen = sf_1.pos - sf_2.pos + DIM;
				sf_2.tlen = -sf_1.tlen;
				sf_1.pnext = sf_2.pos;
				sf_2.pnext = sf_1.pos;
			/*	
				sf_1.rnext = "*";
				
				sf_1.qual = "*";
				

				sf_1.flag = 0;
				
				sf_1.mapq = 0;
				sf_1.pnext = 0;
				*/

				// TODO change to fmt os lib
				samfile << sf_1.qname << '\t' // QNAME
					<< '*' << '\t' // FLAGS
					<< sf_1.rname << '\t' // RNAME
					<< sf_1.pos << '\t' // pos (1-based)
					<< 255 << '\t' // MAPQ
					<< sf_1.cigar << '\t' // CIGAR
					<< '=' << '\t' // MATE NAME
					<< sf_1.pnext << '\t' // MATE pos
					<< sf_1.tlen << '\t' // TLEN
					<< sf_1.seq << '\t' // SEQ
					<< "*\t" << '\n';// QSTR

				samfile << sf_1.qname << '\t' // QNAME
					<< '*' << '\t' // FLAGS
					<< sf_1.rname << '\t' // RNAME
					<< sf_2.pos << '\t' // pos (1-based)
					<< 255 << '\t' // MAPQ
					<< sf_2.cigar << '\t' // CIGAR
					<< '=' << '\t' // MATE NAME
					<< sf_2.pnext << '\t' // MATE pos
					<< sf_2.tlen << '\t' // TLEN
					<< sf_2.seq << '\t' // SEQ
					<< "*\t" << '\n';// QSTR
			}
		}
		read_seq_file_1.close();
		read_seq_file_2.close();
	}

};

