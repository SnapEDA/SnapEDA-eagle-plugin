/*
* Copyright (c) 2012, Alexander I. Mykyta
* All rights reserved.
* 
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met: 
* 
* 1. Redistributions of source code must retain the above copyright notice, this
*    list of conditions and the following disclaimer. 
* 2. Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution. 
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*==================================================================================================
* File History:
* NAME          DATE         COMMENTS
* Alex M.       04/21/2011   born
* 
*=================================================================================================*/

/*========================================= JSON Functions =========================================
The following functions DO NOT parse entire key paths. Instead, these functions can be used
recursively to navigate through JSON data.

string json_GetKeyList(string inp)
	Returns a tab-separated list of keys available at the current level
	Returns "ERROR" if something goes wrong

string json_GetKeyItem(string inp, string key)
	Returns the value of a key:value pair based on the key string
	Returns "NOTFOUND" if the key cannot be found
	Returns "ERROR" if something goes wrong
	
int json_GetArrayCount(string inp)
	Returns the number of values in the array at the current level
	Returns -1 if something goes wrong
	
string json_GetArrayItem(string inp, int array_idx)
	Returns the value in an array indicated by the array_idx
	Returns "ERROR" if something goes wrong or if index is out of bounds
	
//================================================================================================*/

// Returns index of closing quote or brace
// Returns -1 on error
int findMatchingBrace(string inp, int startIdx){
	char ob, cb;
	int depth;
	int i;
	int inplen = strlen(inp);
	
	switch(inp[startIdx]){
		case '"':
			return(strchr(inp,'"',startIdx+1));
		case '[':
			ob = '[';
			cb = ']';
			break;
		case '{':
			ob = '{';
			cb = '}';
			break;
		default:
			return(-1);
	}
	
	depth = 0;
	i=startIdx + 1;
	while(1){
		if(inp[i] == '"'){
			// opening string. Skip contents entirely
			i = strchr(inp,'"',i+1);
			if(i<0) return(-1);
		}else if(inp[i] == ob){
			// Opening Brace
			depth++;
		}else if(inp[i] == cb){
			// Closing Brace
			if(depth == 0){
				return(i);
			}else{
				depth--;
			}
		}
		
		i++;
		
		if(i==inplen) return(-1);
	}
}

//--------------------------------------------------------------------------------------------------
// Idx is the index of the ':' of the current key
// Returns index of next key's starting quote mark
// returns 0 if no more keys
// returns -1 on error
int seekNextKey(string inp,int Idx){
	int inplen = strlen(inp);
	int i;
	
	// seek up to next comma or control char ( '[' '{' or '}' ) Skip quoted strings
	i=Idx;
	while(1){
		if(inp[i] == '"'){
			// Skip quotes
			i = strchr(inp,'"',i+1);
			if(i<0) return(-1);
		}else if((inp[i] == '[') || (inp[i] == '{')){
			i = findMatchingBrace(inp,i);
			if(i<0) return(-1);
		}else if(inp[i] == '}'){
			// reached end of keys
			return(0);
		}else if(inp[i] == ','){
			break;
		}
		
		i++;
		if(i==inplen) return(-1);
	}
	
	// seek to start of next key name
	i = strchr(inp,'"',i);
	if(i<0) return(-1);
	return(i);
	
	
}

//--------------------------------------------------------------------------------------------------
string json_GetKeyList(string inp){
	int idx1, idx2;
	string ret;
	
	if(inp[0] != '{') return("ERROR");
	
	// Find matching key name
	idx1 = strchr(inp,'"');
	if(idx1 < 0) return("ERROR");
	ret = "";
	while(1){
		idx2 = findMatchingBrace(inp,idx1);
		if(idx2 < 0) return("ERROR");
		
		ret += strsub(inp,idx1+1,idx2-idx1-1);
		
		// seek to next key
		idx1 = seekNextKey(inp,idx2+1);
		if(idx1<0){
			return("ERROR");
		}else if(idx1 == 0){
			break;
		}
		ret += '\t';
	}
	
	return(ret);
}

//--------------------------------------------------------------------------------------------------
string json_GetKeyItem(string inp, string key){
	int idx1, idx2;
	
	int ridx1, ridx2; // return selection
	int inplen = strlen(inp);
	
	if(inp[0] != '{') return("ERROR");
	
	// Find matching key name
	idx1 = strchr(inp,'"');
	if(idx1 < 0) return("ERROR");
	while(1){
		idx2 = findMatchingBrace(inp,idx1);
		if(idx2 < 0) return("ERROR");
		
		if(strsub(inp,idx1+1,idx2-idx1-1) == key){
			// Found Key
			idx1 = idx2+2; // seek to char after ':'
			break;
		}
		
		// seek to next key
		idx1 = seekNextKey(inp,idx2+1);
		if(idx1<0){
			return("ERROR");
		}else if(idx1 == 0){
			return("NOTFOUND");
		}
	}
	
	// Found key. 
	// seek to next non-whitespace char
	while((inp[idx1] == ' ') || (inp[idx1] == '\t') || (inp[idx1] == '\n') || (inp[idx1] == '\r')){
		idx1++;
		if(idx1 == inplen) return("ERROR");
	}
	
	switch(inp[idx1]){
		case '{':
		case '[':
		case '"':
			// contains a bracketed item
			ridx1 = idx1;
			ridx2 = findMatchingBrace(inp,idx1);
			if(ridx2 < 0) return("ERROR");
			ridx2++;
			break;
		default:
			// Plain Entry
			ridx1 = idx1;
			
			// seek to a termination char
			ridx2 = ridx1;
			while(1){
				if((inp[ridx2] == ',') || (inp[ridx2] == '}')){
					// term char
					break;
				}
				ridx2++;
				if(ridx2 == inplen) return("ERROR");
			}
			break;
	}
	return(strsub(inp,ridx1,ridx2-ridx1));
}

//--------------------------------------------------------------------------------------------------
int json_GetArrayCount(string inp){
	int idx1;
	int inplen = strlen(inp);
	int count;
	
	if(inp[0] != '[') return(-1);
	
	idx1 = 1;
	
	count = 0;
	while(1){
		// seek to next non-whitespace char
		while((inp[idx1] == ' ') || (inp[idx1] == '\t') || (inp[idx1] == '\n') || (inp[idx1] == '\r')){
			idx1++;
			if(idx1 == inplen) return(-1);
		}
		
		// select array item
		if((inp[idx1] == '[') || (inp[idx1] == '{')){
			// bracketed item
			idx1 = findMatchingBrace(inp,idx1);
			if(idx1 < 0) return(-1);
			idx1++;
			if(inp[idx1] == ','){
				idx1++;
			}
			count++;
		}else if(inp[idx1] == ']'){
			// End of array
			break;
		}else{
			// Single item
			count++;
			// seek to a termination char
			while(1){
				if(inp[idx1] == '"'){
					// skip string contents
					idx1 = strchr(inp,'"',idx1+1);
					if(idx1 < 0) return(-1);
				}
				if(inp[idx1] == ','){
					break;
				}
				if(inp[idx1] == ']'){
					// last item in array
					return(count);
				}
				idx1++;
				if(idx1 == inplen) return(-1);
			}
		}
	}
	return(count);
}

//--------------------------------------------------------------------------------------------------
string json_GetArrayItem(string inp,int array_idx){
	int idx1, idx2;
	int ridx1, ridx2; // return selection
	int inplen = strlen(inp);
	int current_array_idx;
	
	if(array_idx <0) return("ERROR");
	if(inp[0] != '[') return("ERROR");
	if(inp[1] == ']') return("ERROR");
	
	idx1 = 1;
	
	current_array_idx = 0;
	while(1){
		// seek to next non-whitespace char
		while((inp[idx1] == ' ') || (inp[idx1] == '\t') || (inp[idx1] == '\n') || (inp[idx1] == '\r')){
			idx1++;
			if(idx1 == inplen) return("ERROR");
		}
		
		// select array item
		if((inp[idx1] == '[') || (inp[idx1] == '{')){
			// bracketed item
			ridx1 = idx1;
			ridx2 = findMatchingBrace(inp,idx1);
			if(ridx2 < 0) return("ERROR");
			ridx2++;
			if(inp[ridx2] != ','){
				// last item in array
				if(current_array_idx < array_idx){
					return("ERROR");
				}
			}
		}else{
			// Single item
			ridx1 = idx1;
			
			// seek to a termination char
			ridx2 = ridx1;
			while(1){
				if(inp[ridx2] == '"'){
					// skip string contents
					ridx2 = strchr(inp,'"',ridx2+1);
					if(ridx2 < 0) return("ERROR");
				}
				if(inp[ridx2] == ','){
					break;
				}
				if(inp[ridx2] == ']'){
					// last item in array
					if(current_array_idx < array_idx){
						return("ERROR");
					}
					break;
				}
				ridx2++;
				if(ridx2 == inplen) return("ERROR");
			}
		}
		
		if(current_array_idx == array_idx) break;
		current_array_idx++;
		idx1 = ridx2+1;
	}
	return(strsub(inp,ridx1,ridx2-ridx1));
}

//--------------------------------------------------------------------------------------------------

