package edu.cs300;

//ReportField stores all attributes of a report's field
public class ReportField {
	int startIndex;
	int endIndex;
	String title;

	public ReportField(int startIndex, int endIndex, String title) {
		this.startIndex = startIndex; //The first index of the corresponding field
		this.endIndex = endIndex;     //The last index of the corresponding field
		this.title = title;	      //The header of the corresponding field
	}
}
