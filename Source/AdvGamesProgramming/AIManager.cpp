// Fill out your copyright notice in the Description page of Project Settings.


#include "AIManager.h"

#include "EngineUtils.h"
#include "EnemyCharacter.h"
#include "Engine/World.h"
#include "Math/UnrealMathUtility.h"

// Sets default values
AAIManager::AAIManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	AllowedAngle = 0.4f;
	bSteepnessPreventConnection = true;

	Heuristic = EHeuristicType::EUCLIDEAN;
	Pathfinding = EPathfindingType::A_STAR;
}

// Called when the game starts or when spawned
void AAIManager::BeginPlay()
{
	Super::BeginPlay();
	
	PopulateNodes();
	CreateAgents();
}

// Called every frame
void AAIManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

TArray<ANavigationNode*> AAIManager::GeneratePath(ANavigationNode* StartNode, ANavigationNode* EndNode)
{
	if(Pathfinding==EPathfindingType::A_STAR)
	{
		return GenerateAStarPath(StartNode,EndNode);
	}

	if(Pathfinding==EPathfindingType::JPS)
	{
		return  GenerateJPSPath(StartNode,EndNode);
	}
	
	UE_LOG(LogTemp, Error, TEXT("NO PATH FOUND"));
	return TArray<ANavigationNode*>();
}

TArray<ANavigationNode*> AAIManager::GenerateJPSPath(ANavigationNode* StartNode, ANavigationNode* EndNode)
{
	TArray<ANavigationNode*> OpenSet;
	
	for (ANavigationNode* Node : AllNodes)
	{
		Node->GScore = TNumericLimits<float>::Max();
	}

	StartNode->GScore = 0;
	StartNode->HScore = CalculateHeuristic(StartNode->GetActorLocation(), EndNode->GetActorLocation());

	OpenSet.Add(StartNode);
	
	while (OpenSet.Num() > 0)
	{
		// Get node in openset with lowest F-Score, make it current node, remove from open set
		int32 IndexLowestFScore = 0;
		for (int32 i = 1; i < OpenSet.Num(); i++)
		{
			if (OpenSet[i]->FScore() < OpenSet[IndexLowestFScore]->FScore())
			{
				IndexLowestFScore = i;
			}
		}
		ANavigationNode* CurrentNode = OpenSet[IndexLowestFScore];
		OpenSet.Remove(CurrentNode);

		// If end goal reached, back trace to start and return path
		if (CurrentNode == EndNode)
		{
			TArray<ANavigationNode*> Path;
			Path.Push(EndNode);
			CurrentNode = EndNode;
			while (CurrentNode != StartNode)
			{
				CurrentNode = CurrentNode->CameFrom;
				Path.Add(CurrentNode);
			}
			return Path;
		}

		// For the current node, identify the neighbours, see if new G-score better than current one
		
		for (ANavigationNode* ConnectedNode : CurrentNode->ConnectedNodes)
		{
			float TentativeGScore = CurrentNode->GScore + CalculateHeuristic(CurrentNode->GetActorLocation(), ConnectedNode->GetActorLocation());
			if (TentativeGScore < ConnectedNode->GScore)
			{
				ConnectedNode->CameFrom = CurrentNode;
				ConnectedNode->GScore = TentativeGScore;
				ConnectedNode->HScore = CalculateHeuristic(ConnectedNode->GetActorLocation(),EndNode->GetActorLocation());
				if (!OpenSet.Contains(ConnectedNode))
				{
					OpenSet.Add(ConnectedNode);
				}
			}
		}
	}
	UE_LOG(LogTemp, Error, TEXT("NO PATH FOUND"));
	return TArray<ANavigationNode*>();
}

void AAIManager::IdentifyJpsSuccessors(TArray<ANavigationNode*>& OpenSet, ANavigationNode*& CurrentNode, ANavigationNode* EndNode)
{
	// // For the current node, identify the neighbours, see if new G-score better than current one
	// for (ANavigationNode* ConnectedNode : CurrentNode->ConnectedNodes)
	// {
	// 	float TentativeGScore = CurrentNode->GScore + CalculateHeuristic(CurrentNode->GetActorLocation(), ConnectedNode->GetActorLocation());
	// 	if (TentativeGScore < ConnectedNode->GScore)
	// 	{
	// 		ConnectedNode->CameFrom = CurrentNode;
	// 		ConnectedNode->GScore = TentativeGScore;
	// 		ConnectedNode->HScore = CalculateHeuristic(ConnectedNode->GetActorLocation(),EndNode->GetActorLocation());
	// 		if (!OpenSet.Contains(ConnectedNode))
	// 		{
	// 			OpenSet.Add(ConnectedNode);
	// 		}
	// 	}
	// }
}

TArray<ANavigationNode*> AAIManager::GenerateAStarPath(ANavigationNode* StartNode, ANavigationNode* EndNode)
{
	TArray<ANavigationNode*> OpenSet;
	for (ANavigationNode* Node : AllNodes)
	{
		Node->GScore = TNumericLimits<float>::Max();
	}

	StartNode->GScore = 0;
	StartNode->HScore = CalculateHeuristic(StartNode->GetActorLocation(), EndNode->GetActorLocation());

	OpenSet.Add(StartNode);

	while (OpenSet.Num() > 0)
	{
		int32 IndexLowestFScore = 0;
		for (int32 i = 1; i < OpenSet.Num(); i++)
		{
			if (OpenSet[i]->FScore() < OpenSet[IndexLowestFScore]->FScore())
			{
				IndexLowestFScore = i;
			}
		}
		ANavigationNode* CurrentNode = OpenSet[IndexLowestFScore];

		OpenSet.Remove(CurrentNode);

		if (CurrentNode == EndNode) {
			TArray<ANavigationNode*> Path;
			Path.Push(EndNode);
			CurrentNode = EndNode;
			while (CurrentNode != StartNode)
			{
				CurrentNode = CurrentNode->CameFrom;
				Path.Add(CurrentNode);
			}
			return Path;
		}

		for (ANavigationNode* ConnectedNode : CurrentNode->ConnectedNodes)
		{
			float TentativeGScore = CurrentNode->GScore + FVector::Distance(CurrentNode->GetActorLocation(), ConnectedNode->GetActorLocation());
			if (TentativeGScore < ConnectedNode->GScore)
			{
				ConnectedNode->CameFrom = CurrentNode;
				ConnectedNode->GScore = TentativeGScore;
				ConnectedNode->HScore = CalculateHeuristic(ConnectedNode->GetActorLocation(),EndNode->GetActorLocation());
				if (!OpenSet.Contains(ConnectedNode))
				{
					OpenSet.Add(ConnectedNode);
				}
			}
		}
	}

	//If it leaves this loop without finding the end node then return an empty path.
	UE_LOG(LogTemp, Error, TEXT("NO PATH FOUND"));
	return TArray<ANavigationNode*>();
}

float AAIManager::CalculateHeuristic(FVector CurrentNodeLocation, FVector GoalNodeLocation)
{
	// Finds the H-Score value between two nodes based on a chosen Heuristic
	// Euclidean is straightline distance (use for if not on grid)
	// Octile and Chebyshev are 8-direction grid steps between the nodes (Octile Diagonal cost = sqrt(2), Cheby = 1)
	if(Heuristic==EHeuristicType::EUCLIDEAN)
	{
		return FVector::Distance(CurrentNodeLocation, GoalNodeLocation);
	}

	if(Heuristic == EHeuristicType::OCTILE || Heuristic == EHeuristicType::CHEBYSHEV)
	{
		float D1 = 1.0f; // Horizontal/Vertical Cost
		float D2 = Heuristic == EHeuristicType::OCTILE ? FMath::Sqrt(2.0f):1.0f; // Diagonal Move cost
		float Dx = abs(GoalNodeLocation.X - CurrentNodeLocation.X);
		float Dy = abs(GoalNodeLocation.Y - CurrentNodeLocation.Y);
		return D1*(Dx+Dy) + (D2-2*D1)*FMath::Min(Dx,Dy); 
	}

	UE_LOG(LogTemp, Error, TEXT("No Heuristic Set"));
	return TNumericLimits<float>::Max();
}

void AAIManager::PopulateNodes()
{
	AllNodes.Empty();
	AllTraversableNodes.Empty();

	for (TActorIterator<ANavigationNode> It(GetWorld()); It; ++It)
	{
		AllNodes.Add(*It);
		if((*It)->bIsTraversible)
		{
			AllTraversableNodes.Add(*It);
		}
	}
}

void AAIManager::CreateAgents()
{
	for (int32 i = 0; i < NumAI; i++)
	{
		int32 RandIndex = FMath::RandRange(0, AllTraversableNodes.Num()-1);
		AEnemyCharacter* Agent = GetWorld()->SpawnActor<AEnemyCharacter>(AgentToSpawn, AllTraversableNodes[RandIndex]->GetActorLocation(), FRotator(0.f, 0.f, 0.f));
		Agent->Manager = this;
		Agent->CurrentNode = AllTraversableNodes[RandIndex];
		AllAgents.Add(Agent);
	}
}

void AAIManager::GenerateNodes(const TArray<FVector>& Vertices, int32 Width, int32 Height)
{
	AllNodes.Empty();
	AllTraversableNodes.Empty();

	for (TActorIterator<ANavigationNode> It(GetWorld()); It; ++It)
	{
		It->Destroy();
	}

	for (int32 Col = 0; Col < Width; Col++)
	{
		for (int32 Row = 0; Row < Height; Row++)
		{
			//Create and add the nodes to the AllNodes array.
			ANavigationNode* Node = GetWorld()->SpawnActor<ANavigationNode>(Vertices[Row * Width + Col], FRotator::ZeroRotator, FActorSpawnParameters());
			AllNodes.Add(Node);
			Node->GridLocation = FVector2D(Col,Row);
			if(Node->bIsTraversible)
			{
				AllTraversableNodes.Add(Node);
			}
		}
		
	}

	for (int32 Col = 0; Col < Width; Col++)
	{
		for (int32 Row = 0; Row < Height; Row++)
		{
			//Add the connections.

			// CORNER CASES:
			if (Row == 0 && Col == 0)
			{
				//   - Bottom Corner where Row = 0 and Col = 0
				AddConnection(AllNodes[Row * Width + Col], AllNodes[(Row + 1) * Width + Col]);
				AddConnection(AllNodes[Row * Width + Col], AllNodes[(Row + 1) * Width + (Col + 1)]);
				AddConnection(AllNodes[Row * Width + Col], AllNodes[Row * Width + (Col + 1)]);
			}
			else if (Row == 0 && Col == Width - 1)
			{
				//   - Bottom Corner where Row = 0 and Col = Width - 1
				AddConnection(AllNodes[Row * Width + Col], AllNodes[(Row + 1) * Width + Col]);
				AddConnection(AllNodes[Row * Width + Col], AllNodes[(Row + 1) * Width + (Col - 1)]);
				AddConnection(AllNodes[Row * Width + Col], AllNodes[Row * Width + (Col - 1)]);
			}
			else if (Row == Height - 1 && Col == 0)
			{
				//   - Top Corner where Row = Height - 1 and Col = 0
				AddConnection(AllNodes[Row * Width + Col], AllNodes[(Row - 1) * Width + Col]);
				AddConnection(AllNodes[Row * Width + Col], AllNodes[(Row - 1) * Width + (Col + 1)]);
				AddConnection(AllNodes[Row * Width + Col], AllNodes[Row * Width + (Col + 1)]);
			}
			else if (Row == Height - 1 && Col == Width - 1)
			{
				//   - Top Corner where Row = Height - 1 and Col = Width - 1
				AddConnection(AllNodes[Row * Width + Col], AllNodes[(Row - 1) * Width + Col]);
				AddConnection(AllNodes[Row * Width + Col], AllNodes[(Row - 1) * Width + (Col - 1)]);
				AddConnection(AllNodes[Row * Width + Col], AllNodes[Row * Width + (Col - 1)]);
			}
			// EDGE CASES:
			else if (Col == 0)
			{
				//   - Left Edge where Col = 0
				AddConnection(AllNodes[Row * Width + Col], AllNodes[(Row + 1) * Width + Col]);
				AddConnection(AllNodes[Row * Width + Col], AllNodes[(Row + 1) * Width + (Col + 1)]);
				AddConnection(AllNodes[Row * Width + Col], AllNodes[Row * Width + (Col + 1)]);
				AddConnection(AllNodes[Row * Width + Col], AllNodes[(Row - 1) * Width + Col]);
				AddConnection(AllNodes[Row * Width + Col], AllNodes[(Row - 1) * Width + (Col + 1)]);
			}
			else if (Row == Height - 1)
			{
				//   - Top Edge where Row = Height - 1
				AddConnection(AllNodes[Row * Width + Col], AllNodes[Row * Width + (Col - 1)]);
				AddConnection(AllNodes[Row * Width + Col], AllNodes[Row * Width + (Col + 1)]);
				AddConnection(AllNodes[Row * Width + Col], AllNodes[(Row - 1) * Width + (Col - 1)]);
				AddConnection(AllNodes[Row * Width + Col], AllNodes[(Row - 1) * Width + Col]);
				AddConnection(AllNodes[Row * Width + Col], AllNodes[(Row - 1) * Width + (Col + 1)]);
			}
			else if (Col == Width - 1)
			{
				//   - Right Edge where Col = Width - 1
				AddConnection(AllNodes[Row * Width + Col], AllNodes[(Row + 1) * Width + Col]);
				AddConnection(AllNodes[Row * Width + Col], AllNodes[(Row + 1) * Width + (Col - 1)]);
				AddConnection(AllNodes[Row * Width + Col], AllNodes[Row * Width + (Col - 1)]);
				AddConnection(AllNodes[Row * Width + Col], AllNodes[(Row - 1) * Width + Col]);
				AddConnection(AllNodes[Row * Width + Col], AllNodes[(Row - 1) * Width + (Col - 1)]);
			}
			else if (Row == 0)
			{
				//   - Bottom Edge where Row = 0
				AddConnection(AllNodes[Row * Width + Col], AllNodes[Row * Width + (Col - 1)]);
				AddConnection(AllNodes[Row * Width + Col], AllNodes[Row * Width + (Col + 1)]);
				AddConnection(AllNodes[Row * Width + Col], AllNodes[(Row + 1) * Width + (Col - 1)]);
				AddConnection(AllNodes[Row * Width + Col], AllNodes[(Row + 1) * Width + Col]);
				AddConnection(AllNodes[Row * Width + Col], AllNodes[(Row + 1) * Width + (Col + 1)]);
			}
			// NORMAL CASES
			else
			{
				//Connect Top Left
				AddConnection(AllNodes[Row * Width + Col], AllNodes[(Row + 1) * Width + (Col - 1)]);
				//Connect Top
				AddConnection(AllNodes[Row * Width + Col], AllNodes[(Row + 1) * Width + Col]);
				//Connect Top Right
				AddConnection(AllNodes[Row * Width + Col], AllNodes[(Row + 1) * Width + (Col + 1)]);
				//Connect Middle Left
				AddConnection(AllNodes[Row * Width + Col], AllNodes[Row * Width + (Col - 1)]);
				//Connect Middle Right
				AddConnection(AllNodes[Row * Width + Col], AllNodes[Row * Width + (Col + 1)]);
				//Connect Bottom Left
				AddConnection(AllNodes[Row * Width + Col], AllNodes[(Row - 1) * Width + (Col - 1)]);
				//Connect Bottom Middle
				AddConnection(AllNodes[Row * Width + Col], AllNodes[(Row - 1) * Width + Col]);
				//Connect Bottom Right
				AddConnection(AllNodes[Row * Width + Col], AllNodes[(Row - 1) * Width + (Col + 1)]);
			}
		}
	}
}

void AAIManager::AddConnection(ANavigationNode* FromNode, ANavigationNode* ToNode)
{
	bool bCanConnect = true;
	if(bSteepnessPreventConnection)
	{
		FVector Direction = FromNode->GetActorLocation() - ToNode->GetActorLocation();
		Direction.Normalize();
		bCanConnect = (Direction.Z < AllowedAngle && Direction.Z > AllowedAngle * -1.0f);
	}
	
	if (bCanConnect)
	{
		if(!ToNode->bIsTraversible)
		{
			FromNode->ConnectedNonTraversableNodes.Add(ToNode);
		}
		else
		{
			FromNode->ConnectedNodes.Add(ToNode);
		}
		FromNode->AllConnectedNodes.Add(ToNode);

		// For a node, store the directions of all nodes (i.e Right node = (-1,0), Top-Right = (-1,1), etc
		FVector2D Dir2D = ToNode->GridLocation-FromNode->GridLocation;
		FromNode->AllConnectedDir.Add(Dir2D);
	}

}

ANavigationNode* AAIManager::FindNearestNode(const FVector& Location)
{
	ANavigationNode* NearestNode = nullptr;
	float NearestDistance = TNumericLimits<float>::Max();
	//Loop through the nodes and find the nearest one in distance
	for (ANavigationNode* CurrentNode : AllTraversableNodes)
	{
		float CurrentNodeDistance = FVector::Distance(Location, CurrentNode->GetActorLocation());
		if (CurrentNodeDistance < NearestDistance)
		{
			NearestDistance = CurrentNodeDistance;
			NearestNode = CurrentNode;
		}
	}
	UE_LOG(LogTemp, Error, TEXT("Nearest Node: %s"), *NearestNode->GetName())
	return NearestNode;
}

ANavigationNode* AAIManager::FindFurthestNode(const FVector& Location)
{
	ANavigationNode* FurthestNode = nullptr;
	float FurthestDistance = 0.0f;
	//Loop through the nodes and find the nearest one in distance
	for (ANavigationNode* CurrentNode : AllTraversableNodes)
	{
		float CurrentNodeDistance = FVector::Distance(Location, CurrentNode->GetActorLocation());
		if (CurrentNodeDistance > FurthestDistance)
		{
			FurthestDistance = CurrentNodeDistance;
			FurthestNode = CurrentNode;
		}
	}

	UE_LOG(LogTemp, Error, TEXT("Furthest Node: %s"), *FurthestNode->GetName())
	return FurthestNode;
}


