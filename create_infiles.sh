#!/bin/bash

if [ $# != 5 ]
then echo "Usage is ./create_infiles.sh <diseasesFile> <countriesFile> <inputDir> <NumFilesPerDirectory> <NumRecordsPerFile>"
	exit 1
fi


diseaseFile=$1
countriesFile=$2
inputDir=$3
numFilesPerDir=$4
numRecordsPerFile=$5

mkdir $inputDir

NAMES=("Jack" "Mark" "Sam" "Dave" "Boris" "Joe" "David" 
"Bob" "Eddie" "Chris" "Louis" "Andrew" "Melissa" "Kylie" 
"Mary" "Sofia" "Bella" "Christine" "Anna" "Georgia")
#20 values

LASTNAMES=("Smith" "Jones" "Taylor" "Brown" "Williams" "Wilson" 
"Johnson" "Davies" "Robinson" "Wright" "Thompson" "Evans" "Walker" 
"White" "Roberts" "Green" "Hall" "Wood" "Jackson" "Clarke")
#20 values

#echo "${NAMES[19]}"
#echo "${LASTNAMES[19]}"

#Create COUNTRIES array
counter=0
input="./$countriesFile"
while IFS= read -r line
do
	#echo "$line"
	COUNTRIES[$counter]=$line
	counter=`expr $counter + 1`

done < "$input"
#echo "${COUNTRIES[*]}"

#Create DISEASES array
counter=0
input="./$diseaseFile"
while IFS= read -r line
do
        #echo "$line"
        DISEASES[$counter]=$line
        counter=`expr $counter + 1`

done < "$input"
#echo "${DISEASES[*]}"

idcounter=0
exitCounter=0 #Create exit Patient record every 5 records

#Create Directories
for i in "${COUNTRIES[@]}"
do
	dir="./$inputDir/$i"
	mkdir $dir
	#Create files in the directory
	j=0
	while [ $j -lt $numFilesPerDir ]
	do
	    DIFF=$((30-1+1))
        R=$(($(($RANDOM%$DIFF))+1))
		day=$R
        DIFF=$((12-1+1))
        R=$(($(($RANDOM%$DIFF))+1))
        month=$R
        DIFF=$((2020-2000+1))
        R=$(($(($RANDOM%$DIFF))+2000))
        year=$R
		filename="$dir/$day-$month-$year"
		touch $filename
		#Produce a random records
		a=$numRecordsPerFile
		b=0
		while [ $b -lt $a ]
		do
			record_id=$idcounter
			DIFF=$((19-1+1))
			R=$(($(($RANDOM%$DIFF))+1))
			name=${NAMES[$R]}
			DIFF=$((19-1+1))
			R=$(($(($RANDOM%$DIFF))+1))
			surname=${LASTNAMES[$R]}
			DIFF=$((($counter-1)-1+1))
			R=$(($(($RANDOM%$DIFF))+1))
			virus=${DISEASES[$R]}
			DIFF=$((120-1+1))
			R=$(($(($RANDOM%$DIFF))+1))
			age=$R
			if [ $exitCounter == 1 ]
			then
				exitPatientRecord="$record_id EXIT $name $surname $virus $age"
			fi
			if [ $exitCounter == 6 ]
			then
				echo "$exitPatientRecord" >> $filename
				exitCounter=0
			else
				patientRecord="$record_id ENTER $name $surname $virus $age"
				echo "$patientRecord" >> $filename
			fi
			b=`expr $b + 1`
			idcounter=`expr $idcounter + 1`
			exitCounter=`expr $exitCounter + 1`
		done
		j=`expr $j + 1`
	done
done